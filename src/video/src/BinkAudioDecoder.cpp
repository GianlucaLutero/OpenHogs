// BinkAudioDecoder.cpp - see BinkAudioDecoder.h for an overview.
//
// The algorithm follows FFmpeg's libavcodec/binkaudio.c, whose notice is:
//
//   Bink Audio decoder
//   Copyright (c) 2007-2011 Peter Ross (pross@xvid.org)
//   Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu)
//
//   FFmpeg is free software; you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
// This file is part of OpenHogs and distributed under the GNU GPL v3 (the
// LGPL allows that). Changes: written in C++ for OpenHogs; only the RDFT
// variant is decoded; the transform is OpenHogs' own FFT.

#include "openhogs/video/BinkAudioDecoder.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/video/BitReader.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <complex>
#include <numbers>

namespace openhogs::video {

namespace {

// Upper edges of the critical bands of hearing (Hz), as used by WMA; Bink
// quantises each band with its own step.
constexpr std::array<int, 25> kCriticalFrequencies{100,  200,  300,  400,  510,  630,  770,  920,  1080,
                                                   1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400,
                                                   5300, 6400, 7700, 9500, 12000, 15500, 24500};

// Lengths of runs of coefficients sharing a bit width (in groups of 8).
constexpr std::array<int, 16> kRunLengths{2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64};

// A float stored as 5 bits of exponent, 23 bits of mantissa and a sign.
float readPackedFloat(BitReader& bits) {
    const int power = static_cast<int>(bits.read(5));
    float value = std::ldexp(static_cast<float>(bits.read(23)), power - 23);
    if (bits.readBit()) value = -value;
    return value;
}

// In-place inverse complex FFT (no scaling), size a power of two.
void inverseFft(std::vector<std::complex<float>>& values) {
    const std::size_t n = values.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {  // bit-reversal permutation
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(values[i], values[j]);
    }
    for (std::size_t length = 2; length <= n; length <<= 1) {
        const double angle = 2.0 * std::numbers::pi / static_cast<double>(length);
        const std::complex<float> step(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)));
        for (std::size_t start = 0; start < n; start += length) {
            std::complex<float> twiddle(1.0f, 0.0f);
            for (std::size_t k = 0; k < length / 2; ++k) {
                const auto even = values[start + k];
                const auto odd = values[start + k + length / 2] * twiddle;
                values[start + k] = even + odd;
                values[start + k + length / 2] = even - odd;
                twiddle *= step;
            }
        }
    }
}

} // namespace

BinkAudioDecoder::BinkAudioDecoder(const BinkAudioTrack& track, char revision)
    : sampleRate_(track.sampleRate), channels_(track.stereo ? 2 : 1), versionB_(revision == 'b') {
    if (track.usesDct) {
        throw core::FormatError("Bink audio: the DCT variant is not supported");
    }
    if (sampleRate_ == 0) {
        throw core::FormatError("Bink audio: no sample rate");
    }
    int frameLengthBits = sampleRate_ < 22050 ? 9 : (sampleRate_ < 44100 ? 10 : 11);
    // The RDFT variant codes the interleaved samples as one channel at
    // twice the rate.
    const std::uint32_t codedRate = sampleRate_ * static_cast<std::uint32_t>(channels_);
    if (!versionB_ && channels_ == 2) ++frameLengthBits;

    frameLength_ = 1 << frameLengthBits;
    overlapLength_ = frameLength_ / 16;
    root_ = 2.0f / (std::sqrt(static_cast<float>(frameLength_)) * 32768.0f);
    for (int i = 0; i < 96; ++i) {
        // exp(i * 0.0664 / log10(e)): steps of about 0.66 dB.
        quantTable_.push_back(std::exp(static_cast<float>(i) * 0.15289164787221953823f) * root_);
    }

    const auto halfRate = static_cast<int>((codedRate + 1) / 2);
    int bandCount = 1;
    for (; bandCount < 25; ++bandCount) {
        if (halfRate <= kCriticalFrequencies[static_cast<std::size_t>(bandCount - 1)]) break;
    }
    bands_.push_back(2);
    for (int i = 1; i < bandCount; ++i) {
        bands_.push_back((kCriticalFrequencies[static_cast<std::size_t>(i - 1)] * frameLength_ / halfRate) & ~1);
    }
    bands_.push_back(frameLength_);

    previous_.assign(static_cast<std::size_t>(overlapLength_), 0.0f);
}

std::vector<float> BinkAudioDecoder::decode(std::span<const std::byte> packet) {
    std::vector<float> samples;
    if (packet.size() < 4) return samples;
    BitReader bits(packet);
    bits.skip(32);  // the size of the decoded sound, in bytes
    while (bits.bitsLeft() > 0) {
        decodeBlock(bits, samples);
        bits.alignTo32();
    }
    return samples;
}

void BinkAudioDecoder::decodeBlock(BitReader& bits, std::vector<float>& output) {
    const int bandCount = static_cast<int>(bands_.size()) - 1;
    std::vector<float> coefficients(static_cast<std::size_t>(frameLength_) + 2, 0.0f);
    std::vector<float> block(static_cast<std::size_t>(frameLength_), 0.0f);

    // (The RDFT variant has a single coded channel.)
    if (bits.bitsLeft() < 58) throw core::FormatError("Bink audio: packet too short");
    if (versionB_) {
        const auto first = bits.read(32);
        const auto second = bits.read(32);
        coefficients[0] = std::bit_cast<float>(first) * root_;
        coefficients[1] = std::bit_cast<float>(second) * root_;
    } else {
        coefficients[0] = readPackedFloat(bits) * root_;
        coefficients[1] = readPackedFloat(bits) * root_;
    }

    std::vector<float> quant(static_cast<std::size_t>(bandCount));
    for (auto& step : quant) {
        step = quantTable_[std::min<std::uint32_t>(bits.read(8), 95)];
    }

    // The coefficients, in runs that share a bit width; each band of
    // frequencies has its own quantisation step.
    int band = 0;
    float step = quant[0];
    int i = 2;
    while (i < frameLength_) {
        int end = i + 8;
        if (versionB_) {
            end = i + 16;
        } else if (bits.readBit()) {
            end = i + kRunLengths[bits.read(4)] * 8;
        }
        end = std::min(end, frameLength_);

        const int width = static_cast<int>(bits.read(4));
        if (width == 0) {
            std::fill(coefficients.begin() + i, coefficients.begin() + end, 0.0f);
            i = end;
            while (bands_[static_cast<std::size_t>(band)] < i) step = quant[static_cast<std::size_t>(band++)];
        } else {
            for (; i < end; ++i) {
                if (bands_[static_cast<std::size_t>(band)] == i) step = quant[static_cast<std::size_t>(band++)];
                const auto value = static_cast<float>(bits.read(width));
                float coefficient = 0.0f;
                if (value != 0.0f) coefficient = bits.readBit() ? -step * value : step * value;
                coefficients[static_cast<std::size_t>(i)] = coefficient;
            }
        }
        if (bits.bitsLeft() < 0) throw core::FormatError("Bink audio: packet too short");
    }

    inverseRdft(coefficients, block);

    // Cross-fade the start with the end of the previous block.
    auto& previous = previous_;
    if (!first_) {
        const int count = overlapLength_;
        for (int n = 0; n < overlapLength_; ++n) {
            const auto index = static_cast<std::size_t>(n);
            block[index] = (previous[index] * static_cast<float>(count - n) + block[index] * static_cast<float>(n)) /
                           static_cast<float>(count);
        }
    }
    std::copy(block.end() - overlapLength_, block.end(), previous.begin());
    first_ = false;

    output.insert(output.end(), block.begin(), block.end() - overlapLength_);
}

void BinkAudioDecoder::inverseRdft(std::vector<float>& coefficients, std::vector<float>& samples) const {
    // coefficients: [0] the constant term, [1] the highest frequency, then
    // (real, imaginary) pairs for the frequencies 1 .. N/2 - 1. The sound
    // is half the inverse Fourier transform of the symmetric spectrum.
    const auto n = static_cast<std::size_t>(frameLength_);
    std::vector<std::complex<float>> spectrum(n);
    spectrum[0] = {coefficients[0], 0.0f};
    spectrum[n / 2] = {coefficients[1], 0.0f};
    for (std::size_t k = 1; k < n / 2; ++k) {
        const std::complex<float> value(coefficients[2 * k], -coefficients[2 * k + 1]);
        spectrum[k] = value;
        spectrum[n - k] = std::conj(value);
    }
    inverseFft(spectrum);
    for (std::size_t i = 0; i < n; ++i) {
        samples[i] = 0.5f * spectrum[i].real();
    }
}

} // namespace openhogs::video

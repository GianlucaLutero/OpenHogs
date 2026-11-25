// SpeechExtraction.h
//
// Where each line of the PC's speech is on a PlayStation disc, so the
// language pack builder can take the dubbed voices of a localised PAL
// release (decision 139). See docs/formats/psx-speech.md.
//
// The PC keeps one WAV per line, the PlayStation one interleaved XA file
// (XaAudio.h) per set of lines, a line per channel:
//
//   PC Speech/Sku1/PigNN/NNAAGGVV.wav    (hog voice NN, accent AA, group
//                                         GG 1-5, variant VV 1-6)
//   -> disc SPEECH/SKU1/PIGNN/NNAA01.IXA, channel (GG - 1) x 6 + VV - 1
//      (channels 30 and 31 are empty)
//
//   PC Speech/Sku1/Sarge/SGENSSVV.wav    (section SS 1-22, variant VV 1-8)
//   -> line i = (SS - 1) x 8 + VV - 1, 32 to a file:
//      disc SPEECH/SKU1/SARGE/SGEN0<i / 32 + 1>.IXA, channel i mod 32
//
//   PC Speech/Sku1/Train1/tr1_enNN.wav   (the Boot Camp's instructor)
//   -> disc SPEECH/SKU1/TRAIN1/TR1_EN.IXA, channel NN - 1
//
// The mapping was found on the European disc (SLES-01041), whose English
// lines match the PC's one for one in length.

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace openhogs::langpack {

struct SpeechSource {
    std::string discFile;  // e.g. "SPEECH/SKU1/PIG01/01EN01.IXA"
    int channel = 0;       // XA channel in that file

    bool operator==(const SpeechSource&) const = default;
};

// The disc line of a PC speech file, given by its path in the game folder
// ("Speech/Sku1/Pig01/01EN0203.wav", any case, '/' or '\'). Empty for a
// path that is not one of the game's speech lines.
std::optional<SpeechSource> discSourceOf(std::string_view pcPath);

// Whether a disc's boot file (SYSTEM.CNF's BOOT line) is one of the
// localised PAL releases, whose speech is dubbed: SLES-02766 (French),
// 02767 (German), 02768 (Spanish) or 02769 (Italian). The European
// SLES-01041 has the PC's English voices.
bool isDubbedRelease(std::string_view systemCnf);

} // namespace openhogs::langpack

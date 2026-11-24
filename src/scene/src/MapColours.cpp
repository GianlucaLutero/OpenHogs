// MapColours.cpp - see MapColours.h for an overview.

#include "openhogs/scene/MapColours.h"

#include <algorithm>

namespace openhogs::scene {

namespace {

constexpr int kChannelMax = 31;

struct Channels {
    int red = 0;
    int green = 0;
    int blue = 0;

    int grey() const { return (red + green + blue) / 3; }

    // The renderer never leaves a colour black: black is transparent.
    void avoidBlack() {
        if (red == 0 && green == 0 && blue == 0) red = 1;
    }
};

int stepTowards(int value, int target, int steps) {
    if (value < target) return std::min(value + steps, target);
    return std::max(value - steps, target);
}

void desaturate(Channels& c, int steps) {
    const int grey = c.grey();
    if (steps == 5) {
        c.red = c.green = c.blue = grey;
    } else {
        c.red = stepTowards(c.red, grey, steps);
        c.green = stepTowards(c.green, grey, steps);
        c.blue = stepTowards(c.blue, grey, steps);
    }
    c.avoidBlack();
}

void brighten(Channels& c, int amount) {
    c.red = std::clamp(c.red + amount, 0, kChannelMax);
    c.green = std::clamp(c.green + amount, 0, kChannelMax);
    c.blue = std::clamp(c.blue + amount, 0, kChannelMax);
    c.avoidBlack();
}

void tint(Channels& c, const PaletteGrading& grading) {
    const int grey = c.grey();
    const auto tinted = [grey](std::uint8_t tintChannel) {
        return std::min((tintChannel + kTintBase) * grey >> 4, kChannelMax);
    };
    const int percent = grading.tintPercent;
    const auto mix = [percent](int tintedValue, int value) {
        return (tintedValue * percent + (100 - percent) * value) / 100;
    };
    c.red = mix(tinted(grading.tint[0]), c.red);
    c.green = mix(tinted(grading.tint[1]), c.green);
    c.blue = mix(tinted(grading.tint[2]), c.blue);
    c.avoidBlack();
}

} // namespace

std::uint16_t gradeColour(std::uint16_t colour, const PaletteGrading& grading) {
    if (colour == 0) return colour;
    Channels c{colour & 0x1F, (colour >> 5) & 0x1F, (colour >> 10) & 0x1F};

    if (grading.desaturation != 0) desaturate(c, grading.desaturation);
    const int brightness = grading.brightness + kRendererBrightness;
    if (brightness != 0) brighten(c, brightness);
    if (grading.tintPercent > 0) tint(c, grading);

    return static_cast<std::uint16_t>((colour & 0x8000) | (c.blue << 10) | (c.green << 5) | c.red);
}

} // namespace openhogs::scene

// CharacterExpressions.cpp - see CharacterExpressions.h for an overview.

#include "openhogs/scene/CharacterExpressions.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/PathUtils.h"

#include <random>
#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::scene {

namespace {

struct NamedExpression {
    Expression expression;
    std::string_view name;
};

constexpr std::array<NamedExpression, 8> kNames{{
    {Expression::Neutral, "neutral"},
    {Expression::Happy, "happy"},
    {Expression::Sad, "sad"},
    {Expression::Shocked, "shocked"},
    {Expression::Angry, "angry"},
    {Expression::EyesClosed, "eyes-closed"},
    {Expression::Dying, "dying"},
    {Expression::Gormless, "gormless"},
}};

// The built-in rules, by animation number. They follow the animation names
// found in the executable (scene::animationName); animations not listed
// keep a neutral face.
using E = Expression;
constexpr std::pair<std::size_t, Expression> kDefaultRules[] = {
    {1, E::Sad},      {2, E::Sad},                          // running wounded
    {8, E::Shocked},  {9, E::Shocked},                      // jumping
    {11, E::Shocked},                                       // scramble
    {12, E::Angry},   {13, E::Angry},   {14, E::Angry},     // getting out a weapon...
    {15, E::Angry},   {16, E::Angry},   {17, E::Angry},
    {18, E::Angry},   {19, E::Angry},   {20, E::Angry},     // ...and using it
    {21, E::Angry},   {22, E::Angry},   {23, E::Angry},     // bayonet, aiming
    {24, E::Angry},   {25, E::Angry},   {26, E::Angry},
    {29, E::Sad},                                           // very wounded
    {30, E::Happy},                                         // Lord Flash-Heart pose
    {32, E::Gormless},                                      // looking gormless
    {33, E::Shocked},                                       // cowering
    {34, E::Angry},   {35, E::Angry},   {36, E::Angry},     // brushing off (annoyed)
    {37, E::EyesClosed},                                    // sneeze
    {38, E::Shocked}, {39, E::Shocked},                     // flying, bouncing on the behind
    {40, E::Sad},                                           // getting to feet
    {41, E::Happy},   {42, E::Happy},   {43, E::Happy},     // celebrations
    {44, E::Happy},                                         // salute
    {47, E::Dying},   {48, E::Dying},   {49, E::Dying},     // dying
    {50, E::Shocked},                                       // drowning
    {51, E::Sad},     {52, E::Sad},                         // idle cold, idle hot
    {56, E::Angry},                                         // air strike
    {57, E::Sad},                                           // hari kiri
    {58, E::Shocked},                                       // parachuting
};

constexpr std::size_t kAnimationCount = 93;

// Face texture numbers in FACES.MTD (eyes00N / gobs00N) for an expression;
// 0 means the team's default texture.
struct FaceChoice {
    int eyes;
    int mouth;
};

FaceChoice faceFor(Expression expression, bool masked) {
    if (masked) {
        // The balaclava variants only exist open/closed and grin/neutral.
        switch (expression) {
        case E::EyesClosed:
        case E::Dying: return {7, 7};
        case E::Happy: return {6, 6};
        default: return {6, 7};
        }
    }
    switch (expression) {
    case E::Neutral: return {1, 2};
    case E::Happy: return {1, 1};
    case E::Sad: return {3, 3};
    case E::Shocked: return {4, 4};
    case E::Angry: return {5, 5};
    case E::EyesClosed: return {2, 2};
    case E::Dying: return {2, 4};
    case E::Gormless: return {0, 0};
    }
    return {1, 2};
}

int closedEyes(bool masked) {
    return masked ? 7 : 2;
}

std::uint32_t pick(const std::array<std::optional<std::uint32_t>, 8>& slots, int number, std::uint32_t fallback) {
    if (number <= 0 || number >= static_cast<int>(slots.size())) {
        return fallback;
    }
    return slots[static_cast<std::size_t>(number)].value_or(fallback);
}

} // namespace

std::string_view expressionName(Expression expression) {
    for (const auto& named : kNames) {
        if (named.expression == expression) {
            return named.name;
        }
    }
    return "neutral";
}

std::optional<Expression> parseExpression(std::string_view name) {
    const std::string key = core::toLower(core::trim(name));
    for (const auto& named : kNames) {
        if (named.name == key) {
            return named.expression;
        }
    }
    return std::nullopt;
}

ExpressionRules ExpressionRules::defaults() {
    ExpressionRules rules;
    rules.byAnimation_.assign(kAnimationCount, Expression::Neutral);
    for (const auto& [animation, expression] : kDefaultRules) {
        rules.byAnimation_[animation] = expression;
    }
    return rules;
}

void ExpressionRules::applyOverrides(const core::IniFile& ini) {
    for (const auto& [key, value] : ini.entries("Animations")) {
        std::size_t animation = 0;
        try {
            std::size_t used = 0;
            animation = std::stoul(key, &used);
            if (used != key.size()) throw std::invalid_argument(key);
        } catch (const std::exception&) {
            throw core::FormatError("expressions.ini: '" + key + "' is not an animation number");
        }
        const auto expression = parseExpression(value);
        if (!expression) {
            throw core::FormatError("expressions.ini: unknown expression '" + value + "' for animation " + key);
        }
        if (animation >= byAnimation_.size()) {
            byAnimation_.resize(animation + 1, Expression::Neutral);
        }
        byAnimation_[animation] = *expression;
    }
}

Expression ExpressionRules::forAnimation(std::size_t animation) const {
    return animation < byAnimation_.size() ? byAnimation_[animation] : Expression::Neutral;
}

bool isBlinking(double seconds, std::uint32_t seed) {
    constexpr double kBlinkDuration = 0.15;
    constexpr double kSlot = 5.5;  // one blink per slot: on average every 5.5 s

    // Time is cut into slots; each slot holds one blink at a pseudo-random
    // moment. That gives gaps between blinks of varying length without
    // having to remember when the last one was.
    if (seconds < 0.0) {
        return false;
    }
    const auto slot = static_cast<std::uint32_t>(seconds / kSlot);
    // The moment of the blink inside the slot comes from a random number
    // generator seeded with the slot and the hog: always the same answer
    // for the same time and hog, different for different slots and hogs.
    std::seed_seq seeds{slot, seed};
    std::mt19937 random(seeds);
    const double start = static_cast<double>(random() % 1000) / 1000.0 * (kSlot - kBlinkDuration);
    const double inSlot = seconds - static_cast<double>(slot) * kSlot;
    return inSlot >= start && inSlot < start + kBlinkDuration;
}

void applyExpression(Mesh& mesh, const FaceTextureSlots& slots, Expression expression, bool blinking, bool masked) {
    if (!slots.defaultEyes || !slots.defaultMouth) {
        return;
    }
    FaceChoice face = faceFor(expression, masked);
    if (blinking) {
        face.eyes = closedEyes(masked);
    }
    const std::uint32_t eyes = pick(slots.eyes, face.eyes, *slots.defaultEyes);
    const std::uint32_t mouth = pick(slots.mouths, face.mouth, *slots.defaultMouth);

    // Meshes are grouped by texture, so only the two face groups change.
    for (auto& part : mesh) {
        if (part.textureIndex == slots.defaultEyes) {
            part.textureIndex = eyes;
        } else if (part.textureIndex == slots.defaultMouth) {
            part.textureIndex = mouth;
        }
    }
}

} // namespace openhogs::scene

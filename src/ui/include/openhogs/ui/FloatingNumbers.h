// FloatingNumbers.h
//
// The damage numbers of the original game: when a hog is hurt, the points
// it lost appear above it, drift upwards and fade away. The executable
// draws them as particles made of the digit pictures num0 .. num9 of
// Language/Tims/expltims.mad, side by side, as big as the distance to the
// camera allows (see docs/formats/hud.md).
//
// This class only keeps track of the numbers in the world; the game
// application projects them onto the screen and draws the digits.

#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace openhogs::ui {

struct FloatingNumber {
    std::string text;           // the digits
    glm::vec3 position{0.0f};   // where it is now, world units
    glm::vec4 colour{1.0f};     // alpha falls to 0 as it fades
    float ageSeconds = 0.0f;
};

class FloatingNumbers {
public:
    static constexpr float kLifeSeconds = 2.0f;
    static constexpr float kRiseSpeed = 150.0f;  // world units per second

    // A hog at `position` (its head) lost `amount` health points. Amounts
    // under one point are shown as 1, as the hog did get hurt.
    void add(const glm::vec3& position, float amount, const glm::vec4& colour);

    // Moves the numbers up, fades them and forgets those that are gone.
    void update(double seconds);

    const std::vector<FloatingNumber>& numbers() const { return numbers_; }

private:
    std::vector<FloatingNumber> numbers_;
};

// The size of a digit on the canvas for a number `distance` world units
// from the camera: the executable divides by the distance and never goes
// below 10 pixels. OpenHogs caps it at 48 so a number close to the camera
// does not fill the screen.
float floatingDigitSize(float distance);

} // namespace openhogs::ui

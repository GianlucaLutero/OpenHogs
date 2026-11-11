// FloatingNumbers.cpp - see FloatingNumbers.h for an overview.

#include "openhogs/ui/FloatingNumbers.h"

#include <algorithm>
#include <cmath>

namespace openhogs::ui {

namespace {

// A digit is this big (canvas units) at this distance from the camera.
constexpr float kReferenceSize = 24.0f;
constexpr float kReferenceDistance = 1500.0f;
constexpr float kSmallestSize = 10.0f;  // as in the executable
constexpr float kLargestSize = 48.0f;

} // namespace

void FloatingNumbers::add(const glm::vec3& position, float amount, const glm::vec4& colour) {
    FloatingNumber number;
    number.text = std::to_string(std::max(1, static_cast<int>(std::lround(amount))));
    number.position = position;
    number.colour = colour;
    numbers_.push_back(number);
}

void FloatingNumbers::update(double seconds) {
    const auto elapsed = static_cast<float>(seconds);
    for (auto& number : numbers_) {
        number.ageSeconds += elapsed;
        number.position.y += kRiseSpeed * elapsed;
        number.colour.a = std::clamp(1.0f - number.ageSeconds / kLifeSeconds, 0.0f, 1.0f);
    }
    std::erase_if(numbers_, [](const FloatingNumber& number) { return number.ageSeconds >= kLifeSeconds; });
}

float floatingDigitSize(float distance) {
    const float size = kReferenceSize * kReferenceDistance / std::max(distance, 1.0f);
    return std::clamp(size, kSmallestSize, kLargestSize);
}

} // namespace openhogs::ui

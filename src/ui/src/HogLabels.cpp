// HogLabels.cpp - see HogLabels.h for an overview.

#include "openhogs/ui/HogLabels.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace openhogs::ui {

namespace {

// The executable's offsets for a moved label (FUN_00459b20), from the
// centre of the hog's square and its half size: right of it 5 pixels past
// its edge, left of it 75 pixels before; above it 73 pixels up, below 5.
constexpr float kRightGap = 5.0f;
constexpr float kLeftGap = 75.0f;
constexpr float kAboveRise = 73.0f;
constexpr float kBelowDrop = 5.0f;
// A label moved below its hog goes down by the arrow's height more, so the
// arrow fits above its text.
constexpr float kBelowArrowRoom = 30.0f;

ScreenRect labelAbove(glm::vec2 centre, float half) {
    return {{centre.x - kLabelBoxSize.x / 2.0f, centre.y - half - kLabelBoxSize.y}, kLabelBoxSize};
}

ScreenRect labelAt(LabelPlace place, glm::vec2 centre, float half) {
    switch (place) {
        case LabelPlace::Above: return labelAbove(centre, half);
        case LabelPlace::AboveRight: return {{centre.x + half + kRightGap, centre.y - kAboveRise}, kMovedLabelBoxSize};
        case LabelPlace::AboveLeft: return {{centre.x - half - kLeftGap, centre.y - kAboveRise}, kMovedLabelBoxSize};
        case LabelPlace::BelowLeft:
            return {{centre.x - half - kLeftGap, centre.y - kBelowDrop + kBelowArrowRoom}, kMovedLabelBoxSize};
        case LabelPlace::BelowRight:
            return {{centre.x + half + kRightGap, centre.y - kBelowDrop + kBelowArrowRoom}, kMovedLabelBoxSize};
    }
    return labelAbove(centre, half);
}

// The arrow at the corner of the box that faces the hog (the executable's
// drawing loop: 44 x 30 pixels, mirrored by giving it a negative size).
void placeArrow(HogLabelBox& label) {
    const ScreenRect& box = label.box;
    const float right = box.position.x + kLabelBoxSize.x - kLabelArrowSize.x;
    switch (label.place) {
        case LabelPlace::Above: return;
        case LabelPlace::AboveRight:  // the hog is down and to the left
            label.arrow = ScreenRect{{box.position.x, box.position.y + box.size.y - kLabelArrowSize.y}, kLabelArrowSize};
            break;
        case LabelPlace::AboveLeft:  // down and to the right
            label.arrow = ScreenRect{{right, box.position.y + box.size.y - kLabelArrowSize.y}, kLabelArrowSize};
            label.arrowFlipX = true;
            break;
        case LabelPlace::BelowLeft:  // up and to the right
            label.arrow = ScreenRect{{right, box.position.y - kLabelArrowSize.y}, kLabelArrowSize};
            label.arrowFlipX = true;
            label.arrowFlipY = true;
            break;
        case LabelPlace::BelowRight:  // up and to the left
            label.arrow = ScreenRect{{box.position.x, box.position.y - kLabelArrowSize.y}, kLabelArrowSize};
            label.arrowFlipY = true;
            break;
    }
}

// How many other labels and hogs `box` runs into.
int conflicts(const ScreenRect& box, std::size_t self, const std::vector<HogLabelBox>& labels) {
    int count = 0;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i == self) continue;
        if (box.overlaps(labels[i].box)) ++count;
        if (box.overlaps(labels[i].hogRect)) ++count;
    }
    return count;
}

bool isEspionage(const game::Hog& hog) {
    if (hog.rank) return game::rankSpec(*hog.rank).career == game::Career::Espionage;
    const std::string_view id = hog.characterClass ? hog.characterClass->id : std::string_view{};
    return id == "sniper" || id == "spy";
}

// The name of a hog that was given none (the enemies, the skirmish
// teams): the executable's hogs all have a profile with a name, and the
// footage's enemies show the names of their nation's list (LE CONT for
// the French), so the n-th hog of a team takes the list's n-th name.
std::string defaultHogName(const game::World& world, const UiStrings& strings, std::size_t index) {
    const auto& hogs = world.hogs();
    const int nation = world.nationOf(hogs[index].team);
    if (nation < 0 || nation >= static_cast<int>(strings.hogNames.size())) return {};
    const auto& names = strings.hogNames[static_cast<std::size_t>(nation)];
    if (names.empty()) return {};
    std::size_t place = 0;
    for (std::size_t i = 0; i < index; ++i) {
        if (hogs[i].team == hogs[index].team) ++place;
    }
    return names[place % names.size()];
}

} // namespace

bool ScreenRect::overlaps(const ScreenRect& other) const {
    return position.x < other.position.x + other.size.x && other.position.x < position.x + size.x &&
           position.y < other.position.y + other.size.y && other.position.y < position.y + size.y;
}

bool ScreenRect::inside(glm::vec2 screen) const {
    return position.x >= 0.0f && position.y >= 0.0f && position.x + size.x <= screen.x &&
           position.y + size.y <= screen.y;
}

std::vector<HogLabelBox> layoutHogLabels(std::vector<HogOnScreen> hogs, glm::vec2 screen, float focalLength,
                                         bool moveOverlapping) {
    // Farthest first: the nearer labels are drawn over them.
    std::stable_sort(hogs.begin(), hogs.end(),
                     [](const HogOnScreen& a, const HogOnScreen& b) { return a.depth > b.depth; });

    std::vector<HogLabelBox> labels;
    for (const auto& hog : hogs) {
        const float size = kLabelHogScale * (focalLength / kOriginalFocalLength) / std::max(hog.depth, 1.0f);
        HogLabelBox label;
        label.hog = hog.hog;
        label.hogRect = {hog.centre - glm::vec2(size / 2.0f), glm::vec2(size)};
        label.box = labelAbove(hog.centre, size / 2.0f);
        labels.push_back(label);
    }
    if (!moveOverlapping) return labels;

    // A label in the way of another, or off the screen, tries the corners
    // of its hog: the first place clear of everything wins, otherwise the
    // place with the fewest overlaps (the executable's second pass).
    constexpr std::array<LabelPlace, 5> kPlaces{LabelPlace::Above, LabelPlace::AboveRight, LabelPlace::AboveLeft,
                                                LabelPlace::BelowLeft, LabelPlace::BelowRight};
    for (std::size_t i = 0; i < labels.size(); ++i) {
        auto& label = labels[i];
        if (conflicts(label.box, i, labels) == 0 && label.box.inside(screen)) continue;
        const glm::vec2 centre = label.hogRect.position + label.hogRect.size / 2.0f;
        const float half = label.hogRect.size.x / 2.0f;
        int fewest = std::numeric_limits<int>::max();
        for (const auto place : kPlaces) {
            const ScreenRect box = labelAt(place, centre, half);
            int score = conflicts(box, i, labels);
            if (!box.inside(screen)) score += 1;
            if (score < fewest) {
                fewest = score;
                label.place = place;
                label.box = box;
            }
            if (score == 0) break;
        }
        placeArrow(label);
    }
    return labels;
}

std::vector<std::size_t> labelledHogs(const game::World& world) {
    std::vector<std::size_t> labelled;
    const auto& turns = world.turns();
    const std::optional<int> activeTeam = turns ? std::optional(turns->activeTeam()) : std::nullopt;
    for (std::size_t i = 0; i < world.hogs().size(); ++i) {
        const auto& hog = world.hogs()[i];
        // A hog inside a building or vehicle keeps its label, which then
        // names the object (describeHogLabel).
        if (!hog.alive || hog.blownUp || hog.hidden()) continue;
        // Spies and snipers give themselves away only to their own side.
        if (isEspionage(hog) && hog.team != activeTeam) continue;
        labelled.push_back(i);
    }
    return labelled;
}

HogLabelText describeHogLabel(const game::World& world, const UiStrings& strings, std::size_t index,
                              std::optional<float> shownHealth) {
    const auto& hog = world.hogs()[index];
    HogLabelText label;
    label.showHealth = !world.training();
    label.nation = world.nationOf(hog.team);
    if (hog.sheltered()) {
        // Inside, the label names the building or vehicle and shows its
        // health (FUN_0045a5c0: hog states 3 and 4), as on the footage's
        // ARTIGLIERIA 200 (part 17, 30:45).
        const game::Mount& mount = world.mounts()[*hog.mount];
        label.text = strings.objectName(mount.model);
        for (const auto& object : world.destructibles()) {
            if (object.object == mount.object) label.health = std::max(1, static_cast<int>(std::ceil(object.health)));
        }
        return label;
    }
    const std::string rank = hog.rank ? strings.rankName(*hog.rank)
                                      : (hog.characterClass ? strings.className(hog.characterClass->id) : "");
    const std::string name = hog.name.empty() ? defaultHogName(world, strings, index) : hog.name;
    const bool showRank = static_cast<long long>(world.timeSeconds() / kHogLabelSwapSeconds) % 2 == 1;
    label.text = name.empty() || (showRank && !rank.empty()) ? rank : name;
    // Rounded up, so a hog with a sliver of health left never shows 0.
    label.health = std::max(1, static_cast<int>(std::ceil(shownHealth.value_or(hog.health))));
    return label;
}

void HealthCountdown::update(const game::World& world, double seconds) {
    const auto& hogs = world.hogs();
    if (hogs_.size() != hogs.size()) {
        hogs_.assign(hogs.size(), Shown{});
        for (std::size_t i = 0; i < hogs.size(); ++i) hogs_[i].health = hogs[i].health;
        return;
    }
    const auto dt = static_cast<float>(seconds);
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        Shown& shown = hogs_[i];
        if (hogs[i].health >= shown.health) {
            shown = Shown{hogs[i].health, 0.0f};  // healed, or caught up
            continue;
        }
        // The speed grows with the time since the drop began.
        const float before = shown.drainSeconds;
        shown.drainSeconds += dt;
        const float drained = kAcceleration / 2.0f * (shown.drainSeconds * shown.drainSeconds - before * before);
        shown.health = std::max(hogs[i].health, shown.health - drained);
    }
}

float HealthCountdown::shown(const game::World& world, std::size_t index) const {
    return index < hogs_.size() ? hogs_[index].health : world.hogs()[index].health;
}

void HogLabelFade::update(double seconds, bool cameraStill) {
    pending_ += seconds;
    constexpr double kRounding = 1e-9;  // 0.2 s must make five whole ticks
    while (pending_ >= kTickSeconds - kRounding) {
        pending_ -= kTickSeconds;
        level_ = cameraStill ? std::min(kFull, level_ + kRisePerTick) : std::max(0, level_ - kFallPerTick);
    }
}

} // namespace openhogs::ui

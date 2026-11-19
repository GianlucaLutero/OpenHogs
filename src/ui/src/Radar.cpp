// Radar.cpp - see Radar.h for an overview.

#include "openhogs/ui/Radar.h"

#include "openhogs/game/World.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace openhogs::ui {

namespace {

// The surface colours of the renderer's scanner (table at 0x1002BEB8),
// in the order of formats::SurfaceType: mud, grass, metal, wood, water,
// stone, rock, sand, ice, snow, quagmire, lava.
constexpr std::array<glm::ivec3, 12> kSurfaceColours{{
    {60, 50, 40}, {40, 70, 40}, {128, 128, 128}, {153, 94, 34}, {90, 90, 150}, {50, 50, 50},
    {50, 50, 50}, {100, 80, 30}, {180, 240, 240}, {100, 100, 100}, {60, 50, 40}, {240, 100, 53},
}};

// afInitScanner: brightness = 64 + 130 x (height - lowest) / (highest -
// lowest), and each colour channel = colour x brightness / 512, capped at
// 31 (a 5-bit channel of the 16-bit texture).
constexpr float kDarkest = 64.0f;
constexpr float kBrightnessRange = 130.0f;
constexpr int kBrightnessDivisor = 512;
constexpr int kChannelMaximum = 31;

// The hogs' colours (table at 0x1002BE70), by nation; the seventh entry is
// past the table's end, the first surface colour.
constexpr std::array<glm::ivec3, 7> kNationColours{{
    {0, 255, 0}, {96, 255, 255}, {0, 0, 255}, {200, 200, 200}, {255, 0, 0}, {255, 255, 0}, {60, 50, 40},
}};

// The crates' tints (FUN_10009810): weapon crates, health crates,
// promotion points.
constexpr glm::vec4 kWeaponCrateColour{169.0f / 255.0f, 148.0f / 255.0f, 63.0f / 255.0f, 1.0f};
constexpr glm::vec4 kHealthCrateColour{1.0f, 64.0f / 255.0f, 121.0f / 255.0f, 1.0f};
constexpr glm::vec4 kPromotionPointColour{219.0f / 255.0f, 136.0f / 255.0f, 53.0f / 255.0f, 1.0f};

// The scanner is a square plane half kHalfSize wide, kDistance in front of
// a camera with a focal length of 480 (the screen's height), as in the
// renderer (afSetScannerSizeSmall(0) sets 0.151). How steeply it is seen
// comes from the game camera at run time and cannot be read from the
// files: OpenHogs looks down on it at kTilt (decision 154).
constexpr float kHalfSize = 0.151f;
constexpr float kDistance = 1.0f;
constexpr float kFocalLength = 480.0f;
const float kTilt = glm::radians(55.0f);

std::uint8_t toByte(int channel5) {
    return static_cast<std::uint8_t>(channel5 * 255 / kChannelMaximum);
}

} // namespace

formats::RgbaImage radarMapPicture(const game::TerrainHeightField& terrain) {
    constexpr float tile = static_cast<float>(formats::kTileSize);
    const int columns = static_cast<int>(std::lround((terrain.maxX() - terrain.minX()) / tile));
    const int rows = static_cast<int>(std::lround((terrain.maxZ() - terrain.minZ()) / tile));

    // A tile's colour depends on the height of its corner at the lowest X
    // and Z, against the lowest and highest corners of the whole map.
    const auto cornerHeight = [&](int column, int row) {
        return terrain.heightAt(terrain.minX() + static_cast<float>(column) * tile,
                                terrain.minZ() + static_cast<float>(row) * tile);
    };
    float lowest = std::numeric_limits<float>::max();
    float highest = std::numeric_limits<float>::lowest();
    for (int row = 0; row <= rows; ++row) {
        for (int column = 0; column <= columns; ++column) {
            lowest = std::min(lowest, cornerHeight(column, row));
            highest = std::max(highest, cornerHeight(column, row));
        }
    }
    const float range = std::max(highest - lowest, 1.0f);  // a flat map does not divide by zero

    formats::RgbaImage picture;
    picture.width = static_cast<std::uint32_t>(columns);
    picture.height = static_cast<std::uint32_t>(rows);
    picture.pixels.resize(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows) * 4);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float x = terrain.minX() + (static_cast<float>(column) + 0.5f) * tile;
            const float z = terrain.minZ() + (static_cast<float>(row) + 0.5f) * tile;
            const auto& tileData = terrain.tileAt(x, z);
            const int brightness = static_cast<int>(kDarkest + kBrightnessRange * (cornerHeight(column, row) - lowest) / range);
            const auto surface = std::min<std::size_t>(static_cast<std::size_t>(tileData.surface), kSurfaceColours.size() - 1);
            glm::ivec3 colour = kSurfaceColours[surface] * brightness / kBrightnessDivisor;
            if (tileData.isMine) colour = {kChannelMaximum, 0, 0};
            colour = glm::min(colour, glm::ivec3(kChannelMaximum));

            const auto index = (static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) + static_cast<std::size_t>(column)) * 4;
            picture.pixels[index + 0] = toByte(colour.r);
            picture.pixels[index + 1] = toByte(colour.g);
            picture.pixels[index + 2] = toByte(colour.b);
            picture.pixels[index + 3] = 255;
        }
    }
    // The renderer writes the first column with the second one's colour
    // (afInitScanner), so the left edge repeats.
    if (columns > 1) {
        for (int row = 0; row < rows; ++row) {
            const auto first = static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) * 4;
            std::copy_n(picture.pixels.begin() + static_cast<std::ptrdiff_t>(first + 4), 4,
                        picture.pixels.begin() + static_cast<std::ptrdiff_t>(first));
        }
    }
    return picture;
}

glm::vec4 radarHogColour(int nation) {
    const auto index = static_cast<std::size_t>(std::clamp(nation, 0, static_cast<int>(kNationColours.size()) - 1));
    return glm::vec4(glm::vec3(kNationColours[index]) / 255.0f, 1.0f);
}

std::vector<RadarMark> radarMarks(const game::World& world, std::optional<std::size_t> activeHog) {
    const auto& terrain = world.terrain();
    const auto toMap = [&](const glm::vec3& position) {
        return glm::vec2((position.x - terrain.minX()) / (terrain.maxX() - terrain.minX()),
                         (position.z - terrain.minZ()) / (terrain.maxZ() - terrain.minZ()));
    };

    std::vector<RadarMark> marks;
    for (const auto& pickup : world.pickups()) {
        if (pickup.taken || pickup.hidden) continue;
        RadarMark mark;
        mark.mapPoint = toMap(pickup.centre);
        switch (pickup.kind) {
        case game::PickupKind::Health:
            mark.kind = RadarMarkKind::HealthCrate;
            mark.colour = kHealthCrateColour;
            break;
        case game::PickupKind::PromotionPoints:
            mark.kind = RadarMarkKind::PromotionPoint;
            mark.colour = kPromotionPointColour;
            break;
        default:
            mark.kind = RadarMarkKind::WeaponCrate;
            mark.colour = kWeaponCrateColour;
            break;
        }
        marks.push_back(mark);
    }
    // The hogs after the crates, so they are drawn over them.
    for (std::size_t index = 0; index < world.hogs().size(); ++index) {
        const auto& hog = world.hogs()[index];
        if (!hog.alive) continue;
        RadarMark mark;
        mark.mapPoint = toMap(hog.position);
        mark.colour = activeHog == index ? glm::vec4(1.0f) : radarHogColour(world.nationOf(hog.team));
        marks.push_back(mark);
    }
    return marks;
}

RadarView::RadarView(const glm::vec3& cameraForward, const glm::vec2& centre) : centre_(centre) {
    const glm::vec2 ground(cameraForward.x, cameraForward.z);
    if (glm::length(ground) > 1e-4f) forward_ = glm::normalize(ground);
}

glm::vec2 RadarView::project(const glm::vec2& mapPoint) const {
    // The point on the little plane, in the world's directions...
    const glm::vec2 onPlane = (mapPoint - glm::vec2(0.5f)) * (2.0f * kHalfSize);
    // ...then how far ahead of the plane's middle and to the right of it it
    // lies, seen along the camera (right = forward turned a quarter
    // clockwise, seen from above).
    const glm::vec2 right(-forward_.y, forward_.x);
    const float ahead = glm::dot(onPlane, forward_);
    const float across = glm::dot(onPlane, right);
    // Seen from above at kTilt: the far side rises on the screen and
    // shrinks with distance.
    const float depth = kDistance + ahead * std::cos(kTilt);
    const float rise = ahead * std::sin(kTilt);
    return centre_ + glm::vec2(kFocalLength * across / depth, -kFocalLength * rise / depth);
}

std::array<glm::vec2, 4> RadarView::pictureCorners() const {
    return {project({0.0f, 0.0f}), project({1.0f, 0.0f}), project({1.0f, 1.0f}), project({0.0f, 1.0f})};
}

} // namespace openhogs::ui

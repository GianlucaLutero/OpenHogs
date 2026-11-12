// Hog.cpp - see Hog.h for an overview.

#include "openhogs/game/Hog.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/Model.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bit>
#include <string>

namespace openhogs::game {

namespace {

constexpr std::string_view kHogObjectSuffix = "_me";

// Maps also contain CO_ME objects, the commando. The PC release has no
// commando model (only an unused low-detail texture), so until it is known
// what the original draws, commandos use the saboteur's model.
constexpr std::string_view kCommandoPrefix = "co";
constexpr std::string_view kCommandoStandIn = "saboteur";

// A map object's position is the centre of its model, not its base. For
// hogs that centre is about 168 units above the ground: of the 772 hogs in
// the game's maps, 668 are placed 160 to 179 units above the terrain.
constexpr float kSpawnHeightAboveFeet = 168.0f;

// A hog placed this much higher than that stands on something (the highest
// "on the ground" hog of the original maps is 100 units off).
constexpr float kStandsOnObjectMargin = 150.0f;

constexpr std::uint16_t kPlayerFlag = 0x0001;

// Height of a standing hog's body in world units (MapCollision uses the
// same value for its collision cylinder).
constexpr float kHogBodyHeight = 300.0f;

float angleToRadians(std::int16_t angle) {
    return static_cast<float>(angle) * glm::two_pi<float>() / formats::kAngleUnitsPerTurn;
}

} // namespace

const scene::CharacterClass* hogClassForObject(std::string_view modelName) {
    const std::string name = core::toLower(modelName);
    if (!name.ends_with(kHogObjectSuffix)) {
        return nullptr;
    }
    const std::string_view prefix = std::string_view(name).substr(0, name.size() - kHogObjectSuffix.size());
    if (prefix == kCommandoPrefix) {
        return scene::findCharacterClass(kCommandoStandIn);
    }
    for (const auto& characterClass : scene::characterClasses()) {
        if (characterClass.lowModel == prefix) {
            return &characterClass;
        }
    }
    return nullptr;
}

std::vector<Hog> spawnHogs(const std::vector<formats::MapObject>& objects, const TerrainHeightField& terrain) {
    std::vector<Hog> hogs;
    for (const auto& object : objects) {
        const auto* characterClass = hogClassForObject(object.modelName);
        if (characterClass == nullptr) {
            continue;
        }

        Hog hog;
        hog.characterClass = characterClass;
        // One bit per team; a hog without a team bit goes to the first team.
        hog.team = object.team == 0 ? 0 : std::countr_zero(object.team);
        hog.playerControlled = (object.flags & kPlayerFlag) != 0;
        hog.yawRadians = angleToRadians(object.angleY);

        const auto x = static_cast<float>(object.x);
        const auto z = static_cast<float>(object.z);
        const float ground = terrain.heightAt(x, z);
        const float placedFeet = static_cast<float>(object.y) - kSpawnHeightAboveFeet;
        hog.position = {x, placedFeet > ground + kStandsOnObjectMargin ? placedFeet : ground, z};

        // The seed only has to differ from hog to hog.
        hog.idle = IdleAnimator(static_cast<std::uint32_t>(hogs.size()) + 1);
        hog.idle.start(hog.animation);
        const auto loadout = classLoadout(characterClass->id);
        hog.weapons.assign(loadout.begin(), loadout.end());
        hogs.push_back(std::move(hog));
    }
    return hogs;
}

std::string_view teamNation(int team) {
    const auto nations = scene::characterTeams();
    return nations[static_cast<std::size_t>(team < 0 ? 0 : team) % nations.size()];
}

glm::vec3 hogFacing(const Hog& hog) {
    // The model faces +X; this is where the yaw rotation takes that axis.
    return {std::cos(hog.yawRadians), 0.0f, -std::sin(hog.yawRadians)};
}

float yawFacing(const glm::vec3& direction) {
    // The inverse of hogFacing: facing = (cos yaw, 0, -sin yaw).
    return std::atan2(-direction.z, direction.x);
}

bool swimsSafely(const Hog& hog) {
    if (hog.characterClass == nullptr) return false;
    const std::string_view id = hog.characterClass->id;
    return id == "ace" || id == "legend";
}

glm::vec3 hogCentre(const Hog& hog) {
    return hog.position + glm::vec3(0.0f, kHogBodyHeight / 2.0f, 0.0f);
}

glm::mat4 hogModelToWorld(const Hog& hog) {
    const float scale = formats::kModelToWorldScale;
    // The feet are kStandingFeetDepth model units below the model origin,
    // so the origin goes that far (scaled) above the ground.
    glm::mat4 transform = glm::translate(glm::mat4(1.0f),
                                         hog.position + glm::vec3(0.0f, scene::kStandingFeetDepth * scale, 0.0f));
    transform = glm::rotate(transform, hog.yawRadians, glm::vec3(0, 1, 0));
    // Same conversion as map objects: half size, Y mirrored (model Y is down).
    return glm::scale(transform, glm::vec3(scale, -scale, scale));
}

} // namespace openhogs::game

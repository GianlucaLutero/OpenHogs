// Objectives.cpp - see Objectives.h for an overview.

#include "openhogs/game/Objectives.h"

#include <algorithm>
#include <cmath>

namespace openhogs::game {

namespace {

// Script types, as named in Objectives.h.
constexpr int kDestroyForCrate = 1;
constexpr int kDestroyForPoint = 2;
constexpr int kDestroyCostsPoints = 4;
constexpr int kReachForCrate = 7;
constexpr int kReachForPoint = 8;
constexpr int kReachCostsPoints = 10;
constexpr int kFirstReachType = 7;
constexpr int kLastReachType = 10;
constexpr int kGroupForCrate = 13;
constexpr int kGroupForPoint = 14;
constexpr int kGroupCostsPoints = 16;
constexpr int kFirstGroupType = 13;
constexpr int kLastGroupType = 18;
constexpr int kLastObjectiveType = 18;

constexpr int kHealthItem = 255;

// The sergeant's sections (FUN_0043b850 calls in the two functions).
constexpr int kSergeantWeaponCrate = 4;   // destroyed: "here's a weapon"
constexpr int kSergeantHealthCrate = 5;   // destroyed or reached: health
constexpr int kSergeantPoint = 6;         // destroyed: a promotion point
constexpr int kSergeantPointsLost = 7;    // destroyed what should have stayed
constexpr int kSergeantReachedCrate = 8;  // reached: a weapon crate
constexpr int kSergeantReachedPoint = 9;  // reached: a promotion point

// Half the size of the crate models, from their boxes in the maps (every
// CRATE1, CRATE2 and PROPOINT of the original maps has the same box).
constexpr glm::vec3 kWeaponCrateHalfSize{96.0f, 96.0f, 128.0f};
constexpr glm::vec3 kHealthCrateHalfSize{64.0f, 64.0f, 128.0f};
constexpr glm::vec3 kPointHalfSize{96.0f, 192.0f, 96.0f};

// The original grid: 512-unit cells, the map's corner at -16384.
constexpr int kCellShift = 9;
constexpr int kMapHalfWidth = 0x4000;

bool isReachType(int type) { return type >= kFirstReachType && type <= kLastReachType; }
bool isGroupType(int type) { return type >= kFirstGroupType && type <= kLastGroupType; }

// A crate standing on the ground at `ground`.
Pickup crateOnGround(PickupKind kind, const glm::vec3& ground) {
    Pickup pickup;
    pickup.kind = kind;
    pickup.dropped = true;
    switch (kind) {
    case PickupKind::Weapon:
        pickup.model = "CRATE1";
        pickup.halfSize = kWeaponCrateHalfSize;
        break;
    case PickupKind::Health:
        pickup.model = "CRATE2";
        pickup.halfSize = kHealthCrateHalfSize;
        break;
    case PickupKind::PromotionPoints:
        pickup.model = "PROPOINT";
        pickup.halfSize = kPointHalfSize;
        break;
    }
    pickup.centre = ground + glm::vec3(0.0f, pickup.halfSize.y, 0.0f);
    return pickup;
}

// The weapon or health crate of types 1, 7 and 13. Empty when it holds a
// weapon OpenHogs does not know (the executable's table has gaps).
std::optional<Pickup> rewardCrate(const Objective& objective, const glm::vec3& ground) {
    if (objective.item == kHealthItem) {
        Pickup crate = crateOnGround(PickupKind::Health, ground);
        crate.amount = objective.amount;
        return crate;
    }
    if (objective.item < 0) return std::nullopt;
    Pickup crate = crateOnGround(PickupKind::Weapon, ground);
    crate.item = objective.item;
    crate.amount = objective.amount;
    return crate;
}

// A promotion point. The original gives one point a pickup, whatever its
// parameters say (decision 110).
Pickup rewardPoint(const glm::vec3& ground) {
    Pickup point = crateOnGround(PickupKind::PromotionPoints, ground);
    point.amount = 1;
    return point;
}

// The reward of a destroy or group objective, dropped at its target.
ObjectiveResult destroyReward(const Objective& objective) {
    ObjectiveResult result;
    switch (objective.type) {
    case kDestroyForCrate:
    case kGroupForCrate:
        result.drop = rewardCrate(objective, objective.target);
        result.sergeantSection = objective.item == kHealthItem ? kSergeantHealthCrate : kSergeantWeaponCrate;
        break;
    case kDestroyForPoint:
    case kGroupForPoint:
        result.drop = rewardPoint(objective.target);
        result.sergeantSection = kSergeantPoint;
        break;
    case kDestroyCostsPoints:
    case kGroupCostsPoints:
        result.pointsLost = objective.item;
        result.sergeantSection = kSergeantPointsLost;
        break;
    default:
        break;  // counts in its group, gives nothing
    }
    return result;
}

} // namespace

std::vector<Objective> findObjectives(const std::vector<formats::MapObject>& objects) {
    std::vector<Objective> objectives;
    for (std::size_t index = 0; index < objects.size(); ++index) {
        const auto& object = objects[index];
        if (object.scriptType < 1 || object.scriptType > kLastObjectiveType) continue;
        Objective objective;
        objective.object = index;
        objective.type = object.scriptType;
        objective.group = object.scriptGroup;
        objective.item = object.scriptParams[0];
        objective.amount = object.scriptParams[1];
        // Crate rewards hold an item number, turned into a weapon as the
        // map loader does (FUN_004a5250, the table at 0x4D9DC8).
        const bool cratesItem = objective.type == kDestroyForCrate || objective.type == kReachForCrate ||
                                objective.type == kGroupForCrate;
        if (cratesItem && objective.item != kHealthItem) {
            objective.item = crateItemWeapon(objective.item);
        }
        objective.target = {object.scriptX, object.scriptY, object.scriptZ};
        objectives.push_back(objective);
    }
    return objectives;
}

int objectiveCell(float coordinate) {
    // As the original: the position as a 16-bit number, moved so the map
    // starts at 0, divided into 512-unit cells.
    const int position = static_cast<int>(std::floor(coordinate));
    return (position + kMapHalfWidth) >> kCellShift;
}

std::optional<ObjectiveResult> reachObjective(Objective& objective, const glm::vec3& feet) {
    if (objective.done || !isReachType(objective.type)) return std::nullopt;
    if (objectiveCell(feet.x) != objectiveCell(objective.target.x) ||
        objectiveCell(feet.z) != objectiveCell(objective.target.z)) {
        return std::nullopt;
    }
    objective.done = true;

    ObjectiveResult result;
    switch (objective.type) {
    case kReachForCrate:
        result.drop = rewardCrate(objective, feet);
        result.sergeantSection = objective.item == kHealthItem ? kSergeantHealthCrate : kSergeantReachedCrate;
        break;
    case kReachForPoint:
        result.drop = rewardPoint(feet);
        result.sergeantSection = kSergeantReachedPoint;
        break;
    case kReachCostsPoints:
        result.pointsLost = objective.item;  // the original says nothing here
        break;
    default:
        break;  // type 9: nothing is known to happen
    }
    return result;
}

std::optional<ObjectiveResult> destroyObjective(std::vector<Objective>& objectives, std::size_t object) {
    const auto found = std::find_if(objectives.begin(), objectives.end(),
                                    [&](const Objective& o) { return o.object == object && !o.done; });
    if (found == objectives.end()) return std::nullopt;
    found->done = true;
    if (isReachType(found->type)) return std::nullopt;  // its hog died before getting there

    if (isGroupType(found->type)) {
        // Any other objective of the group still standing holds the reward
        // back (FUN_004aa170 looks at every object with a script, whatever
        // its type).
        const int group = found->group;
        const bool othersLeft = std::any_of(objectives.begin(), objectives.end(),
                                            [&](const Objective& o) { return !o.done && o.group == group; });
        if (othersLeft) return std::nullopt;
    }
    return destroyReward(*found);
}

} // namespace openhogs::game

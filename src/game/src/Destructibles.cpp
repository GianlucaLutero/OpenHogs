// Destructibles.cpp - see Destructibles.h for an overview.

#include "openhogs/game/Destructibles.h"

#include "openhogs/core/PathUtils.h"

#include <algorithm>
#include <array>
#include <string>

namespace openhogs::game {

namespace {

struct NamedHealth {
    std::string_view name;
    float health;
};

// Buildings (object table entries 1-6) and their health (0x4C2E08).
constexpr std::array<NamedHealth, 6> kBuildings{{
    {"BIG_GUN", 200.0f}, {"M_TENT1", 30.0f}, {"M_TENT2", 40.0f},
    {"PILLBOX", 100.0f}, {"SHELTER", 100.0f}, {"TENT_S", 25.0f},
}};

// Vehicles (entries 445-451) and their health (0x4D94A0).
constexpr std::array<NamedHealth, 7> kVehicles{{
    {"AMLAUNCH", 150.0f}, {"AM_TANK", 150.0f}, {"BOAT", 150.0f}, {"TANK", 150.0f},
    {"CARRY", 150.0f}, {"AIRSHIP1", 1.0f}, {"PP1", 1.0f},
}};

// Scenery (entries 28-379) whose health differs from the usual 60 points
// (0x4D6D18). Interoperability data, read from the executable.
constexpr float kUsualSceneryHealth = 60.0f;
constexpr std::array<NamedHealth, 105> kScenery{{
    // Shooting-range targets and dummies: one hit.
    {"TARGET", 1.0f}, {"TARGET2", 1.0f}, {"TARGET3", 1.0f}, {"TARGET4", 1.0f}, {"TARGET5", 1.0f},
    {"T_SUP", 1.0f}, {"T_SUP2", 1.0f}, {"T_SUP3", 1.0f}, {"DUMMY", 1.0f},
    {"DUGOUT", 10.0f}, {"DUGOUT_S", 10.0f}, {"PROP", 10.0f}, {"WE_CHUT2", 10.0f},
    {"BIGLOONY", 20.0f}, {"CHECKB", 40.0f}, {"CRATE4", 40.0f},
    // Bridges, blocks, bushes, small trees, flowers and grass.
    {"BRIDG_C2", 50.0f}, {"BRIDGE_C", 50.0f}, {"BRIDGE_S", 50.0f}, {"BRID2_C", 50.0f}, {"BRID2_C1", 50.0f},
    {"BRID2_C2", 50.0f}, {"BRID2C3", 50.0f}, {"BRID2_S", 50.0f}, {"BRID2_S2", 50.0f}, {"D_BRID", 50.0f},
    {"TBLOK", 50.0f}, {"TBLOK2", 50.0f}, {"TBLOK3", 50.0f}, {"BUSH1", 50.0f}, {"BUSH2", 50.0f},
    {"BUSH3", 50.0f}, {"STUMP1", 50.0f}, {"STUMP2", 50.0f}, {"BUSH1H", 50.0f}, {"BUSH2H", 50.0f},
    {"BUSH3H", 50.0f}, {"TREEBH", 50.0f}, {"TREEBH1", 50.0f}, {"TREEGH", 50.0f}, {"TREEGH1", 50.0f},
    {"TREEGH2", 50.0f}, {"TREEPH", 50.0f}, {"TREEPH1", 50.0f}, {"TREEPH2", 50.0f}, {"TREEPAH", 50.0f},
    {"TREEPAH1", 50.0f}, {"TREEPAH2", 50.0f}, {"TREEWH", 50.0f}, {"TREEWH1", 50.0f}, {"FLOWERS", 50.0f},
    {"FLOWERS2", 50.0f}, {"FLOWERS3", 50.0f}, {"GRASS", 50.0f}, {"GRASS1", 50.0f}, {"GRASS2", 50.0f},
    // Trees and cacti.
    {"TREEB", 80.0f}, {"TREEG", 80.0f}, {"TREEP", 80.0f}, {"TREEPA", 80.0f}, {"TREEW", 80.0f},
    {"CACTUS", 80.0f}, {"CACT2", 80.0f},
    // Walls, gates, machines, windmills, radars...
    {"MONO", 100.0f}, {"SIGN", 100.0f}, {"WALL", 100.0f}, {"IRONGATE", 100.0f}, {"BOG", 100.0f},
    {"FENCE", 100.0f}, {"GATE", 100.0f}, {"GATES", 100.0f}, {"LAMP", 100.0f}, {"MACHI", 100.0f},
    {"MOONS", 100.0f}, {"RELAX", 100.0f}, {"RELAX1", 100.0f}, {"SNOWB", 100.0f}, {"SNOWH", 100.0f},
    {"TV", 100.0f}, {"BARREL", 100.0f}, {"PILLER", 100.0f}, {"BARBWIRE", 100.0f}, {"BARBWIR2", 100.0f},
    {"BYRD21", 100.0f}, {"BYRD22", 100.0f}, {"BUTTFLY1", 100.0f}, {"WINDM", 100.0f}, {"WIN_SH", 100.0f},
    {"WIN_SV", 100.0f}, {"CHIM", 100.0f}, {"WIND2", 100.0f}, {"WIND2H", 100.0f}, {"WIND2V", 100.0f},
    {"SWILL1", 100.0f}, {"SWILLARM", 100.0f}, {"SWILL2", 100.0f}, {"SW2ARM", 100.0f}, {"PIST", 100.0f},
    {"WATSTA", 100.0f}, {"WATWHE", 100.0f}, {"CHIM2", 100.0f}, {"RADAR", 100.0f}, {"RADAR1", 100.0f},
    {"TUN", 150.0f}, {"BOOTS", 200.0f},
}};

// Other scenery with the usual health that does not follow the house
// naming below.
constexpr std::array<std::string_view, 5> kOtherUsualScenery{"CHECKP", "GRAV2", "GRAVE", "LU_B_SU", "MAST"};

// The houses, walls and roofs of the villages are named by a style (S1,
// ST, BR, M1, SN, CA, W1), a part (F floor, R roof, S stairs, W wall) and
// a number, e.g. "M1W06PPP"; they all have the usual health.
bool isHousePart(std::string_view name) {
    constexpr std::array<std::string_view, 7> kStyles{"S1", "ST", "BR", "M1", "SN", "CA", "W1"};
    constexpr std::string_view kParts = "FRSW";
    if (name.size() < 4) return false;
    const bool knownStyle = std::find(kStyles.begin(), kStyles.end(), name.substr(0, 2)) != kStyles.end();
    return knownStyle && kParts.find(name[2]) != std::string_view::npos;
}

template <std::size_t N>
std::optional<float> lookUp(const std::array<NamedHealth, N>& table, std::string_view name) {
    for (const auto& entry : table) {
        if (entry.name == name) return entry.health;
    }
    return std::nullopt;
}

// The class and default health of a model name; health is empty for
// anything that is not a building, vehicle or scenery.
std::optional<float> defaultHealth(std::string_view name, ObjectClass& objectClassOut) {
    if (const auto health = lookUp(kBuildings, name)) {
        objectClassOut = ObjectClass::Building;
        return health;
    }
    if (const auto health = lookUp(kVehicles, name)) {
        objectClassOut = ObjectClass::Vehicle;
        return health;
    }
    objectClassOut = ObjectClass::Scenery;
    if (const auto health = lookUp(kScenery, name)) return health;
    if (std::find(kOtherUsualScenery.begin(), kOtherUsualScenery.end(), name) != kOtherUsualScenery.end() ||
        isHousePart(name)) {
        return kUsualSceneryHealth;
    }
    objectClassOut = ObjectClass::Other;
    return std::nullopt;
}

} // namespace

ObjectClass objectClass(std::string_view modelName) {
    ObjectClass result = ObjectClass::Other;
    defaultHealth(core::toUpper(modelName), result);
    return result;
}

std::optional<float> objectHealth(const formats::MapObject& object) {
    ObjectClass kind = ObjectClass::Other;
    auto health = defaultHealth(core::toUpper(object.modelName), kind);
    if (!health) return std::nullopt;
    if (kind == ObjectClass::Scenery && (object.presence & formats::kIndestructibleBit) != 0) {
        return std::nullopt;
    }
    if (object.health != formats::kDefaultHealth) {
        health = static_cast<float>(object.health);  // the map's own value (0 included)
    }
    return health;
}

std::vector<Destructible> findDestructibles(const std::vector<formats::MapObject>& objects) {
    std::vector<Destructible> targets;
    for (std::size_t index = 0; index < objects.size(); ++index) {
        const auto health = objectHealth(objects[index]);
        if (!health) continue;
        Destructible target;
        target.object = index;
        target.objectClass = objectClass(objects[index].modelName);
        target.health = *health;
        targets.push_back(target);
    }
    return targets;
}

bool damageObject(Destructible& target, float amount) {
    if (target.destroyed || amount <= 0.0f) return false;
    target.health -= amount;
    if (target.health >= 1.0f) return false;
    target.health = 0.0f;
    target.destroyed = true;
    return true;
}

} // namespace openhogs::game

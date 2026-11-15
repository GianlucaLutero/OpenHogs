// Pickups.cpp - see Pickups.h for an overview.

#include "openhogs/game/Pickups.h"

#include "openhogs/game/MapCollision.h"  // kHogRadius

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace openhogs::game {

namespace {

// warhogs_.exe at 0x4D9DC8: a crate's item number -> weapon number (text
// entry - 96). 255: no weapon. Interoperability data, read from the
// executable (docs/formats/maps.md).
constexpr std::array<int, 80> kCrateItems{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,  11,  12,  14,  15,  16,  17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 28, 29, 31, 32, 33,  34,  35,  36,  37,  39,  40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 68, 69, 70, 71, 72,  50,  51,  52,  53,  54,  56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 38, 55, 13, 27,  30,  126, 255, 255, 255, 131, 255, 255, 255};

constexpr int kHealthItem = 255;
constexpr float kTouchMargin = kHogRadius + 20.0f;  // the hog stops at the box, its body touching it

std::string upperCase(std::string text) {
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

} // namespace

int crateItemWeapon(int item) {
    if (item < 0 || item >= static_cast<int>(kCrateItems.size())) return -1;
    const int weapon = kCrateItems[static_cast<std::size_t>(item)];
    return weapon == kHealthItem ? -1 : weapon;
}

std::optional<WeaponId> Pickup::weapon() const {
    if (kind != PickupKind::Weapon) return std::nullopt;
    const int id = item + 96;
    for (const auto& spec : allWeapons()) {
        if (static_cast<int>(spec.id) == id) return spec.id;
    }
    return std::nullopt;  // not made yet in OpenHogs (decision 22)
}

std::vector<Pickup> findPickups(const std::vector<formats::MapObject>& objects) {
    std::vector<Pickup> pickups;
    for (std::size_t index = 0; index < objects.size(); ++index) {
        const auto& object = objects[index];
        if (object.scriptType != kPickupScriptType) continue;
        const std::string model = upperCase(object.modelName);
        // Signs carry crate parameters too (an item that is not a weapon);
        // they stay in place (decision 109).
        if (model == "SIGN") continue;
        Pickup pickup;
        pickup.object = index;
        pickup.centre = {object.x, object.y, object.z};
        const float half = formats::kBoxSizeUnit / 2.0f;
        pickup.halfSize = {std::max<float>(object.boxSizeX, 1) * half, std::max<float>(object.boxSizeY, 1) * half,
                           std::max<float>(object.boxSizeZ, 1) * half};
        pickup.amount = object.scriptParams[1];
        const int item = object.scriptParams[0];
        if (model == "PROPOINT") {
            pickup.kind = PickupKind::PromotionPoints;
        } else if (item == kHealthItem) {
            pickup.kind = PickupKind::Health;
        } else {
            pickup.kind = PickupKind::Weapon;
            pickup.item = crateItemWeapon(item);
            if (pickup.item < 0) continue;  // nothing OpenHogs knows
        }
        pickups.push_back(pickup);
    }
    return pickups;
}

bool touches(const Pickup& pickup, const glm::vec3& feet) {
    // The crate's footprint, widened by a margin, and the hog not far
    // above or below it. The box is taken unturned: crates are squarish.
    const float reach = std::max(pickup.halfSize.x, pickup.halfSize.z) + kTouchMargin;
    const glm::vec2 offset{feet.x - pickup.centre.x, feet.z - pickup.centre.z};
    const float bottom = pickup.centre.y - pickup.halfSize.y - kTouchMargin;
    const float top = pickup.centre.y + pickup.halfSize.y + kTouchMargin;
    return std::abs(offset.x) <= reach && std::abs(offset.y) <= reach && feet.y >= bottom - 200.0f && feet.y <= top;
}

} // namespace openhogs::game

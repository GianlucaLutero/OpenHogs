// Hud.cpp - see Hud.h for an overview.

#include "openhogs/ui/Hud.h"

#include "openhogs/ui/TextLayout.h"

#include <algorithm>
#include <cmath>

namespace openhogs::ui {

std::string weaponDisplayName(game::WeaponId weapon, const std::vector<std::string>& gameText) {
    const auto index = static_cast<std::size_t>(weapon);
    if (index < gameText.size() && !gameText[index].empty()) {
        return gameText[index];
    }
    return std::string(game::weaponSpec(weapon).fallbackName);
}

std::optional<HudInfo> describeTurn(const game::World& world, const UiStrings& strings,
                                    const std::vector<std::string>& gameText) {
    const auto& turns = world.turns();
    if (!turns || world.phase() == game::MatchPhase::Over) {
        return std::nullopt;
    }
    const auto& hog = world.hogs()[turns->activeHog()];

    HudInfo info;
    info.team = hog.team;
    info.nation = world.nationOf(hog.team);
    info.teamName = strings.teamName(info.nation);
    info.className = hog.rank ? strings.rankName(*hog.rank) : strings.className(hog.characterClass->id);
    info.hogName = hog.name;
    // Rounded up, so a hog with a sliver of health left never shows 0.
    info.health = hog.alive ? std::max(1, static_cast<int>(std::ceil(hog.health))) : 0;
    info.healthFraction = hog.maxHealth > 0.0f ? std::clamp(hog.health / hog.maxHealth, 0.0f, 1.0f) : 0.0f;
    info.weapon = hog.selectedWeapon();
    info.weaponName = weaponDisplayName(info.weapon, gameText);
    info.exactSecondsLeft = std::max(0.0, turns->secondsLeft());
    info.secondsLeft = static_cast<int>(std::ceil(info.exactSecondsLeft));
    if (hog.charging) {
        info.power = std::clamp(hog.power, 0.0f, 1.0f);
    }
    const auto& spec = game::weaponSpec(info.weapon);
    info.aimDegrees = hog.aimDegrees;
    info.canAim = spec.kind == game::WeaponKind::Bullet || spec.kind == game::WeaponKind::Thrown ||
                  spec.kind == game::WeaponKind::Shell;
    info.usesPower = spec.usesPower;
    info.aimLimited = spec.lowestAimDegrees > 0.0f;
    info.playing = world.phase() == game::MatchPhase::Playing;
    info.turnNumber = turns->turnNumber();
    return info;
}

std::optional<std::string> turnMessage(const game::World& world, const UiStrings& strings, bool computerTurn) {
    const auto& turns = world.turns();
    if (!turns) {
        return std::nullopt;
    }
    if (computerTurn) {
        return strings.enemyTurn;
    }
    return fillPlaceholders(strings.getReady, {strings.teamName(world.nationOf(turns->activeTeam()))});
}

std::optional<std::string> messageForEvent(const game::GameEvent& event, const game::World& world,
                                           const UiStrings& strings, bool computerTurn) {
    switch (event.type) {
    case game::GameEventType::TurnStarted:
        return turnMessage(world, strings, computerTurn);
    case game::GameEventType::SuddenDeath:
        return strings.suddenDeath;
    case game::GameEventType::MatchOver:
        if (event.team < 0) {
            return strings.draw;
        }
        return fillPlaceholders(strings.victory, {strings.teamName(world.nationOf(event.team))});
    default:
        return std::nullopt;
    }
}

std::optional<std::string> pickupMessage(const game::GameEvent& event, const game::World& world,
                                        const UiStrings& strings, const std::vector<std::string>& gameText) {
    if (event.type != game::GameEventType::PickedUp || event.pickup >= world.pickups().size()) return std::nullopt;
    const auto& pickup = world.pickups()[event.pickup];
    switch (pickup.kind) {
    case game::PickupKind::Health: {
        // ">S FINDS USEFUL PROVISIONS!": the hog's name, or its team's.
        if (event.hog >= world.hogs().size()) return std::nullopt;
        const auto& hog = world.hogs()[event.hog];
        const std::string who = hog.name.empty() ? strings.teamName(world.nationOf(hog.team)) : hog.name;
        return fillPlaceholders(strings.findsProvisions, {who});
    }
    case game::PickupKind::Weapon: {
        // The weapon's name and how many ("%u X%d"); just the name in the
        // Boot Camp, where crate weapons never run out ("%u").
        if (!pickup.weapon()) return std::nullopt;
        std::string text = weaponDisplayName(*pickup.weapon(), gameText);
        if (!world.training()) text += " X" + std::to_string(pickup.amount);
        return text;
    }
    case game::PickupKind::PromotionPoints:
        return std::nullopt;  // the executable writes nothing
    }
    return std::nullopt;
}

} // namespace openhogs::ui

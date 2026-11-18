// BattleAwards.cpp - see BattleAwards.h for an overview.

#include "openhogs/game/BattleAwards.h"

#include <algorithm>
#include <cmath>

namespace openhogs::game {

namespace {

// The thresholds of FUN_00483cc0 (kills in one turn, damage in points).
constexpr int kAnnihilatorKills = 4;
constexpr int kTripleKills = 3;
constexpr int kDoubleKills = 2;
constexpr int kMasterSadistDamage = 351;
constexpr int kSadismDamage = 176;

// The executable skips the kill awards and MASTER SADIST when the best hog
// is in team 1: it compares the team with 1 where -1 ("nobody") seems
// meant. The slip is copied, as the owner chose (decision 116).
constexpr int kTeamSkippedByTheExecutable = 1;

struct Candidate {
    std::optional<std::size_t> hog;
    int value = 0;
};

SpecialAward award(SpecialAwardKind kind, std::size_t hog) {
    int points = 0;
    switch (kind) {
    case SpecialAwardKind::Annihilator: points = 5; break;
    case SpecialAwardKind::TripleRipReaper: points = 3; break;
    case SpecialAwardKind::MasterSadist: points = 3; break;
    case SpecialAwardKind::DoubleDeathDealer: points = 2; break;
    case SpecialAwardKind::SadismAward: points = 2; break;
    case SpecialAwardKind::PigOfTheBattle: points = 1; break;
    }
    return {kind, hog, points};
}

// The award of the match, if anyone earned one.
std::optional<SpecialAward> chooseAward(const std::vector<Hog>& hogs, const std::vector<int>& teams) {
    // The best killer needs at least two kills in a turn; the best
    // damage dealer any damage. Only a strictly better hog takes over.
    Candidate killer{std::nullopt, kDoubleKills - 1};
    Candidate damager{std::nullopt, 0};
    for (const int team : teams) {
        for (std::size_t i = 0; i < hogs.size(); ++i) {
            if (hogs[i].team != team) continue;
            const HogRecord& record = hogs[i].record;
            const int kills = std::max(record.mostKillsInATurn, record.killsThisTurn);
            if (kills > killer.value) killer = {i, kills};
            const int damage = static_cast<int>(std::floor(record.enemyDamage));
            if (damage > damager.value) damager = {i, damage};
        }
    }

    if (killer.hog && hogs[*killer.hog].team == kTeamSkippedByTheExecutable) killer.hog.reset();
    const bool masterSadistSkipped = damager.hog && hogs[*damager.hog].team == kTeamSkippedByTheExecutable;

    if (killer.hog && killer.value >= kAnnihilatorKills) return award(SpecialAwardKind::Annihilator, *killer.hog);
    if (killer.hog && killer.value >= kTripleKills) return award(SpecialAwardKind::TripleRipReaper, *killer.hog);
    if (damager.hog && damager.value >= kMasterSadistDamage && !masterSadistSkipped) {
        return award(SpecialAwardKind::MasterSadist, *damager.hog);
    }
    if (killer.hog) return award(SpecialAwardKind::DoubleDeathDealer, *killer.hog);
    if (damager.hog) {
        return award(damager.value < kSadismDamage ? SpecialAwardKind::PigOfTheBattle : SpecialAwardKind::SadismAward,
                     *damager.hog);
    }
    return std::nullopt;
}

} // namespace

BattleResults battleResults(const std::vector<Hog>& hogs, const std::vector<int>& teams, std::optional<int> winner) {
    BattleResults results;
    results.award = chooseAward(hogs, teams);
    for (const int team : teams) {
        TeamResult result;
        result.team = team;
        for (const auto& hog : hogs) {
            if (hog.team != team) continue;
            result.enemiesKilled += hog.record.enemiesKilled;
            result.friendsKilled += hog.record.friendsKilled;
        }
        if (winner == team) result.victoryBonus = kVictoryBonus;
        if (results.award && hogs[results.award->hog].team == team) result.awardPoints = results.award->points;
        results.teams.push_back(result);
    }

    // The best total wins; a share of it means nobody does.
    int best = 0;
    for (std::size_t i = 0; i < results.teams.size(); ++i) {
        const int total = results.teams[i].total();
        if (i == 0 || total > best) {
            best = total;
            results.bestTeam = results.teams[i].team;
        } else if (total == best) {
            results.bestTeam.reset();
        }
    }
    return results;
}

} // namespace openhogs::game

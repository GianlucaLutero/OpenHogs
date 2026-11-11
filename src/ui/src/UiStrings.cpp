// UiStrings.cpp - see UiStrings.h for an overview.

#include "openhogs/ui/UiStrings.h"

#include <cctype>
#include <utility>

namespace openhogs::ui {

namespace {

// Entry numbers in fetext.
constexpr std::size_t kFeYes = 6;
constexpr std::size_t kFeNo = 7;
constexpr std::size_t kFeMainMenu = 8;
constexpr std::size_t kFeOnePlayer = 9;
constexpr std::size_t kFeMultiPlayer = 10;
constexpr std::size_t kFeOptions = 11;
constexpr std::size_t kFeQuit = 16;
constexpr std::size_t kFeVolumeControl = 74;
constexpr std::size_t kFeMaster = 76;
constexpr std::size_t kFeEffects = 77;
constexpr std::size_t kFeVoices = 78;
constexpr std::size_t kFeSelectLevel = 105;
constexpr std::size_t kFeStartGame = 134;
constexpr std::size_t kFeSelectTeam = 24;
constexpr std::size_t kFeStartMission = 50;
constexpr std::size_t kFeNewGame = 55;
constexpr std::size_t kFeLoadGame = 56;
constexpr std::size_t kFePromote = 71;
constexpr std::size_t kFeSwapPosition = 72;
constexpr std::size_t kFeCareerPath = 90;
constexpr std::size_t kFeDeathMatch = 109;
// HEAVY WEAPONS 91, ENGINEER 92, ESPIONAGE 93, MEDIC 94, in UiStrings::careers order.
constexpr std::array<std::size_t, 4> kFeCareers{91, 94, 93, 92};
// Each nation's block: the team name, then its nine hog names.
constexpr std::array<std::size_t, 7> kFeHogNameBlocks{166, 186, 176, 216, 206, 196, 226};
constexpr std::size_t kHogNamesPerNation = 9;
constexpr std::size_t kFePlayer = 330;
constexpr std::size_t kFeCpu = 331;
constexpr std::size_t kFeBack = 350;
constexpr std::size_t kFeLoading = 465;
constexpr std::size_t kFeMusic = 648;
// Team names, in scene::characterTeams() order.
constexpr std::array<std::size_t, 7> kFeTeamNames{25, 27, 26, 30, 29, 28, 226};

// Entry numbers in gtext.
constexpr std::size_t kGtVictory = 165;
constexpr std::size_t kGtDraw = 166;
constexpr std::size_t kGtGetReady = 168;
constexpr std::size_t kGtEnemyTurn = 169;
constexpr std::size_t kGtMissionTitle = 158;
constexpr std::size_t kGtMissionResults = 187;
constexpr std::size_t kGtMissionFailed = 188;
constexpr std::size_t kGtMissionAborted = 189;
constexpr std::size_t kGtLevelComplete = 190;
constexpr std::size_t kGtSurvivalBonus = 191;
constexpr std::size_t kGtRetry = 193;
constexpr std::size_t kGtEditSquad = 194;
constexpr std::size_t kGtRip = 271;
constexpr std::size_t kGtPaused = 173;
constexpr std::size_t kGtAbortSkirmish = 180;
constexpr std::size_t kGtContinue = 181;
constexpr std::size_t kGtAreYouSure = 182;
// Class names: the game names each rank; OpenHogs' classes use the rank
// their model stands for (heavy weapons -> "GUNNER").
const std::vector<std::pair<std::string, std::size_t>> kGtClassNames{
    {"ace", 78},    {"grunt", 63},    {"heavy", 64},  {"legend", 79}, {"medic", 75},
    {"sapper", 68}, {"saboteur", 70}, {"sniper", 72}, {"spy", 73}};

bool isBlank(const std::string& text) {
    for (const char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// Replaces `target` with entry `index` of `table`, if there is a real one.
void take(std::string& target, const std::vector<std::string>& table, std::size_t index) {
    if (index < table.size() && !isBlank(table[index])) {
        target = table[index];
    }
}

} // namespace

const std::string& UiStrings::teamName(int team) const {
    const auto count = static_cast<int>(teamNames.size());
    return teamNames[static_cast<std::size_t>(((team % count) + count) % count)];
}

std::string UiStrings::className(std::string_view classId) const {
    for (const auto& [id, name] : classNames) {
        if (id == classId) return name;
    }
    std::string upper(classId);
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return upper;
}

std::string UiStrings::missionName(const game::Mission& mission) const {
    const auto found = missionNames.find(mission.nameEntry);
    return found != missionNames.end() ? found->second : "MISSION " + std::to_string(mission.number);
}

std::string UiStrings::rankName(game::RankId rank) const {
    const auto ranks = game::allRanks();
    for (std::size_t i = 0; i < ranks.size(); ++i) {
        if (ranks[i].id == rank) {
            return i < rankNames.size() ? rankNames[i] : std::string(ranks[i].fallbackName);
        }
    }
    return {};
}

UiStrings loadUiStrings(const std::vector<std::string>& frontEndText, const std::vector<std::string>& gameText) {
    UiStrings strings;
    const auto& fe = frontEndText;
    take(strings.yes, fe, kFeYes);
    take(strings.no, fe, kFeNo);
    take(strings.mainMenu, fe, kFeMainMenu);
    take(strings.onePlayer, fe, kFeOnePlayer);
    take(strings.multiPlayer, fe, kFeMultiPlayer);
    take(strings.options, fe, kFeOptions);
    take(strings.quit, fe, kFeQuit);
    take(strings.volumeControl, fe, kFeVolumeControl);
    take(strings.master, fe, kFeMaster);
    take(strings.effects, fe, kFeEffects);
    take(strings.voices, fe, kFeVoices);
    take(strings.selectLevel, fe, kFeSelectLevel);
    take(strings.back, fe, kFeBack);
    take(strings.loading, fe, kFeLoading);
    take(strings.music, fe, kFeMusic);
    take(strings.startGame, fe, kFeStartGame);
    take(strings.player, fe, kFePlayer);
    take(strings.cpu, fe, kFeCpu);
    take(strings.selectTeam, fe, kFeSelectTeam);
    take(strings.startMission, fe, kFeStartMission);
    take(strings.newGame, fe, kFeNewGame);
    take(strings.loadGame, fe, kFeLoadGame);
    take(strings.promote, fe, kFePromote);
    take(strings.swapPosition, fe, kFeSwapPosition);
    take(strings.careerPath, fe, kFeCareerPath);
    take(strings.skirmish, fe, kFeDeathMatch);
    for (std::size_t i = 0; i < kFeCareers.size(); ++i) {
        take(strings.careers[i], fe, kFeCareers[i]);
    }
    for (std::size_t nation = 0; nation < kFeHogNameBlocks.size(); ++nation) {
        auto& names = strings.hogNames[nation];
        for (std::size_t i = 0; i < kHogNamesPerNation && i < names.size(); ++i) {
            take(names[i], fe, kFeHogNameBlocks[nation] + 1 + i);
        }
    }
    for (std::size_t team = 0; team < kFeTeamNames.size(); ++team) {
        take(strings.teamNames[team], fe, kFeTeamNames[team]);
    }

    const auto& gt = gameText;
    take(strings.victory, gt, kGtVictory);
    take(strings.draw, gt, kGtDraw);
    take(strings.getReady, gt, kGtGetReady);
    take(strings.enemyTurn, gt, kGtEnemyTurn);
    take(strings.missionTitle, gt, kGtMissionTitle);
    take(strings.missionResults, gt, kGtMissionResults);
    take(strings.missionFailed, gt, kGtMissionFailed);
    take(strings.missionAborted, gt, kGtMissionAborted);
    take(strings.levelComplete, gt, kGtLevelComplete);
    take(strings.survivalBonus, gt, kGtSurvivalBonus);
    take(strings.retry, gt, kGtRetry);
    take(strings.editSquad, gt, kGtEditSquad);
    take(strings.rip, gt, kGtRip);
    for (const auto& mission : game::campaignMissions()) {
        if (mission.nameEntry < gt.size() && !isBlank(gt[mission.nameEntry])) {
            strings.missionNames[mission.nameEntry] = gt[mission.nameEntry];
        }
    }
    take(strings.paused, gt, kGtPaused);
    take(strings.abortSkirmish, gt, kGtAbortSkirmish);
    take(strings.continueGame, gt, kGtContinue);
    take(strings.areYouSure, gt, kGtAreYouSure);
    for (const auto& rank : game::allRanks()) {
        std::string name(rank.fallbackName);
        take(name, gt, rank.textEntry);
        strings.rankNames.push_back(name);
    }
    for (auto& [id, name] : strings.classNames) {
        for (const auto& [classId, index] : kGtClassNames) {
            if (classId == id) take(name, gt, index);
        }
    }
    return strings;
}

} // namespace openhogs::ui

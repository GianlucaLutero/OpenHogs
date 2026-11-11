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
constexpr std::size_t kFeBack = 350;
constexpr std::size_t kFeLoading = 465;
constexpr std::size_t kFeMusic = 648;
// Team names, in scene::characterTeams() order.
constexpr std::array<std::size_t, 7> kFeTeamNames{25, 27, 26, 30, 29, 28, 226};

// Entry numbers in gtext.
constexpr std::size_t kGtVictory = 165;
constexpr std::size_t kGtDraw = 166;
constexpr std::size_t kGtGetReady = 168;
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
    for (std::size_t team = 0; team < kFeTeamNames.size(); ++team) {
        take(strings.teamNames[team], fe, kFeTeamNames[team]);
    }

    const auto& gt = gameText;
    take(strings.victory, gt, kGtVictory);
    take(strings.draw, gt, kGtDraw);
    take(strings.getReady, gt, kGtGetReady);
    take(strings.paused, gt, kGtPaused);
    take(strings.abortSkirmish, gt, kGtAbortSkirmish);
    take(strings.continueGame, gt, kGtContinue);
    take(strings.areYouSure, gt, kGtAreYouSure);
    for (auto& [id, name] : strings.classNames) {
        for (const auto& [classId, index] : kGtClassNames) {
            if (classId == id) take(name, gt, index);
        }
    }
    return strings;
}

} // namespace openhogs::ui

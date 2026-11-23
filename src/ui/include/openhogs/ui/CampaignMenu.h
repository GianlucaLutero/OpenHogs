// CampaignMenu.h
//
// The campaign pages between missions:
//
//     (NEW GAME, a team) ──> PLEASE NAME YOUR TEAM ──> SQUAD
//     (LOAD GAME) ──> LOAD: the eight slots ──> SQUAD
//
//     SQUAD ── a hog ──> the hog: PROMOTE ──> the ranks it can reach (CAREER PATH for a grunt)
//       │                         SWAP POSITION ──> the hog to swap places with
//       │                         RENAME ──> PLEASE NAME YOUR PIG
//       ├── START MISSION ──> (briefing, then the mission)
//       ├── SAVE TEAM ──> SAVE ARMY: the eight slots
//       └── (Esc) ──> (main menu)
//
//     MISSION RESULTS ── CONTINUE ──> SQUAD   (or the main menu when the campaign is won)
//                     ── RETRY / EDIT SQUAD (after a failed or aborted mission)
//
// The squad page lists the eight hogs with their ranks; the ones marked
// "*" go on the next mission (the first five, three in mission 1), so
// swapping a reserve with one of them changes the team.
//
// The menu changes the campaign directly (promotions, swaps) and tells the
// application when to save it, start the mission or leave.

#pragma once

#include "openhogs/game/Campaign.h"
#include "openhogs/ui/FrontEnd.h"  // MenuSound
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/NameEntry.h"
#include "openhogs/ui/UiStrings.h"

#include <functional>
#include <string>
#include <vector>

namespace openhogs::ui {

// The promotion points of the NAUGHTY PIGS cheat.
inline constexpr int kNaughtyPigsPoints = 99;

enum class CampaignScreen { Squad, Hog, Promotion, Swap, Results, Load, Save, NameTeam, NameHog };

// The original's numbered saves (savearmy0 ... savearmy7).
inline constexpr std::size_t kSaveSlots = 8;

// What one of the eight save slots holds, for the loading and saving boards.
struct SaveSlot {
    bool used = false;
    std::string name;     // the team saved there
    int nextMission = 0;  // its next mission
    std::vector<game::RankId> squad;  // its hogs' ranks
};

struct CampaignMenuResult {
    enum class Request {
        None,
        Changed,       // the squad changed: save the campaign
        StartMission,  // play the campaign's next mission
        Leave,         // back to the main menu
        SaveToSlot,    // save the campaign in `slot`
        LoadFromSlot,  // load the campaign in `slot`
    };
    Request request = Request::None;
    std::size_t slot = 0;
    MenuSound sound = MenuSound::None;
};

// How a mission went, for the results page.
struct MissionSummary {
    int missionNumber = 0;
    bool aborted = false;
    game::MissionOutcome outcome;
};

class CampaignMenu {
public:
    // How many hogs go on a mission (it depends on the mission's map).
    using DeploymentRule = std::function<std::size_t(const game::Campaign&)>;

    // Without a rule, kDeployedHogs (5) go on every mission.
    explicit CampaignMenu(UiStrings strings, DeploymentRule deployment = {});

    // The squad page for the next mission.
    void showSquad(const game::Campaign& campaign);
    void showResults(const game::Campaign& campaign, const MissionSummary& summary);
    // The loading board (LOAD GAME): the slots, the cursor on the first
    // used one. Empty slots cannot be chosen.
    void showLoad(std::vector<SaveSlot> slots);
    // PLEASE NAME YOUR TEAM, for a new campaign: the nation's team name to
    // start from.
    void showNameTeam(const game::Campaign& campaign);
    // The slots changed on disk (after saving): shown on the saving board.
    void setSlots(std::vector<SaveSlot> slots);

    CampaignMenuResult handle(MenuInput input, game::Campaign& campaign);
    // The loading board needs no campaign (there is none yet).
    CampaignMenuResult handleLoad(MenuInput input);

    // Typing a name (NameTeam, NameHog): a letter from the keyboard, and
    // Backspace. They do nothing on other pages.
    bool typingName() const { return screen_ == CampaignScreen::NameTeam || screen_ == CampaignScreen::NameHog; }
    CampaignMenuResult type(char letter, game::Campaign& campaign);
    CampaignMenuResult erase();

    // The results page drawn as the original's debriefing picture has two
    // keys printed on it: after a victory, SPACE continues (and so does
    // ESCAPE, whose RETRY OpenHogs does not offer: the victory is already
    // recorded); after a defeat, SPACE retries and ESCAPE edits the squad.
    // Confirm stands for SPACE, Back for ESCAPE; other keys do nothing.
    CampaignMenuResult answerDebrief(MenuInput input, game::Campaign& campaign);

    CampaignScreen screen() const { return screen_; }
    const Menu& menu() const { return menu_; }
    // Information shown under the page's title (mission, points...).
    const std::vector<std::string>& lines() const { return lines_; }
    // The squad page as last shown, and its lines: the original draws a
    // hog's page over it.
    const Menu& squadMenu() const { return squad_; }
    const std::vector<std::string>& squadLines() const { return squadLines_; }
    // The hog whose page is open (or being promoted or swapped).
    std::size_t hogIndex() const { return hog_; }
    // The name being typed (NameTeam, NameHog).
    const NameEntry& nameEntry() const { return nameEntry_; }
    // On the squad page: moves the cursor to the first hog.
    void pointAtFirstHog() {
        if (screen_ == CampaignScreen::Squad) menu_.select(0);
    }
    // The slots (Load, Save), in the menu's order.
    const std::vector<SaveSlot>& slots() const { return slots_; }

private:
    CampaignMenuResult handleSquad(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleHog(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handlePromotion(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleSwap(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleResults(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult chooseResult(int chosen, game::Campaign& campaign);
    CampaignMenuResult handleSave(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleName(NameEntry::Result result, game::Campaign& campaign);
    void showSave(const game::Campaign& campaign);
    void showNameHog(const game::Campaign& campaign);
    Menu slotMenu(const std::string& title, bool emptyAllowed) const;
    void showHog(const game::Campaign& campaign);
    void showPromotion(const game::Campaign& campaign);
    void showSwap(const game::Campaign& campaign);
    std::string pointsLine(int points) const;

    UiStrings strings_;
    CampaignScreen screen_ = CampaignScreen::Squad;
    Menu menu_;
    std::vector<std::string> lines_;
    Menu squad_;
    std::vector<std::string> squadLines_;
    DeploymentRule deployment_;
    std::size_t deployed_ = 0;
    std::size_t hog_ = 0;  // the hog being promoted or swapped
    bool resultsCompleted_ = false;
    NameEntry nameEntry_;
    std::vector<SaveSlot> slots_;
    bool campaignWon_ = false;
};

} // namespace openhogs::ui

// CampaignMenu.h
//
// The campaign pages between missions:
//
//     SQUAD ── a hog ──> the hog: PROMOTE ──> the ranks it can reach (CAREER PATH for a grunt)
//       │                         SWAP POSITION ──> the hog to swap places with
//       ├── START MISSION ──> (briefing, then the mission)
//       └── BACK ──> (main menu; the campaign is saved)
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
#include "openhogs/ui/UiStrings.h"

#include <functional>
#include <string>
#include <vector>

namespace openhogs::ui {

enum class CampaignScreen { Squad, Hog, Promotion, Swap, Results };

struct CampaignMenuResult {
    enum class Request {
        None,
        Changed,       // the squad changed: save the campaign
        StartMission,  // play the campaign's next mission
        Leave,         // back to the main menu
    };
    Request request = Request::None;
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

    CampaignMenuResult handle(MenuInput input, game::Campaign& campaign);

    CampaignScreen screen() const { return screen_; }
    const Menu& menu() const { return menu_; }
    // Information shown under the page's title (mission, points...).
    const std::vector<std::string>& lines() const { return lines_; }

private:
    CampaignMenuResult handleSquad(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleHog(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handlePromotion(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleSwap(const MenuResponse& response, game::Campaign& campaign);
    CampaignMenuResult handleResults(const MenuResponse& response, game::Campaign& campaign);
    void showHog(const game::Campaign& campaign);
    void showPromotion(const game::Campaign& campaign);
    void showSwap(const game::Campaign& campaign);
    std::string pointsLine(int points) const;

    UiStrings strings_;
    CampaignScreen screen_ = CampaignScreen::Squad;
    Menu menu_;
    std::vector<std::string> lines_;
    DeploymentRule deployment_;
    std::size_t deployed_ = 0;
    std::size_t hog_ = 0;  // the hog being promoted or swapped
    bool resultsCompleted_ = false;
    bool campaignWon_ = false;
};

} // namespace openhogs::ui

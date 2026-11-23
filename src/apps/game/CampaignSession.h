// CampaignSession.h
//
// A campaign being played: the game::Campaign, its pages between missions
// (ui::CampaignMenu), the mission briefing, and the link to the matches:
//
//     squad page ── START MISSION ──> briefing ── Enter ──> mission (a Game)
//         ▲                                                   │
//         └──────────── mission results <─────────────────────┘
//
// The campaign is saved to a file after every change (promotion, swap,
// mission), so quitting never loses progress.
//
// Briefings are the game's pictures Language/Tims/Briefing/level<N>.bmp
// (translated by the language packs); mission 1 also shows a photo of a
// hog in the player's uniform, level1n<nation>.bmp, in its frame.

#pragma once

#include "Game.h"
#include "Interface.h"
#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Campaign.h"
#include "openhogs/game/Debrief.h"
#include "openhogs/formats/BitmapImage.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/CampaignMenu.h"

#include <filesystem>
#include <map>
#include "openhogs/game/GameVersion.h"

#include <memory>
#include <optional>
#include <string>

namespace openhogs::gameapp {

class CampaignSession {
public:
    // `rules`: with the PlayStation's, winning the campaign starts its hard
    // mode (game/GameVersion.h).
    CampaignSession(const core::VirtualFileSystem& vfs, Interface& userInterface, std::filesystem::path savePath,
                    game::VersionRules rules = {});

    static bool hasSave(const std::filesystem::path& savePath);

    // Starts a new campaign (replacing any saved one) or continues the
    // saved one; both open the squad page. loadSaved() returns false, with
    // a message on the console, if the file cannot be read.
    void startNew(int nation);
    bool loadSaved();

    enum class Action { None, StartMission, Leave };
    Action handleInput(ui::MenuInput input, SoundPlayer& sounds);

    // The options of the next mission's match.
    GameOptions missionOptions();

    // A mission's match has ended (`game` is over or was aborted): records
    // it and shows the results page. Returns the videos the original plays
    // at that point (an island conquered, the campaign won), if any.
    std::vector<std::string> missionEnded(const Game& game);

    // Draws the current page over whatever is behind (between
    // renderer().begin() and end()).
    void draw();
    // Counts down the newspaper's time on screen.
    void update(double seconds);
    // A full-screen picture (the briefing or the newspaper) hides the menus.
    bool showingPicture() const { return briefing_ != nullptr || newspaper_.has_value() || showingDebrief(); }
    // The campaign's pages (for the original screens), if a campaign is open.
    const ui::CampaignMenu* menu() const { return menu_.get(); }
    // How many hogs the squad has.
    std::size_t squadSize() const { return campaign_ ? campaign_->squad().size() : 0; }

private:
    void save() const;
    void openBriefing();
    bool showingDebrief() const;
    void openDebrief(const std::vector<game::SquadMember>& squad, const game::MissionReport& report,
                     const game::MissionOutcome& outcome, std::size_t deployed);
    void drawDebrief();
    void drawMedal(float x, float y, bool won);
    // A picture of Language/Tims/Debrief/, loaded once (nullptr if missing).
    const render::UiImage* debriefPicture(const std::string& file, formats::BitmapTransparency transparency);
    void openNewspaper(const game::MissionReport& report, const game::MissionOutcome& outcome, int missionNumber);
    std::size_t deploymentFor(const game::Campaign& campaign);

    const core::VirtualFileSystem& vfs_;
    Interface& interface_;
    std::filesystem::path savePath_;
    game::VersionRules rules_;
    std::optional<game::Campaign> campaign_;
    std::unique_ptr<ui::CampaignMenu> menu_;
    std::map<std::string, std::size_t> slotsByMap_;  // places for the player's team, per map

    // The briefing being shown.
    std::unique_ptr<render::UiImage> briefing_;
    std::unique_ptr<render::UiImage> briefingPhoto_;

    // The newspaper printed after a won mission (game/Newspaper.h): the
    // paper with the story and the photo laid on it, until a key is
    // pressed or its time runs out.
    struct NewspaperShown {
        std::unique_ptr<render::UiImage> paper;
        std::unique_ptr<render::UiImage> story;
        std::unique_ptr<render::UiImage> picture;
        double secondsLeft = 0.0;
    };
    std::optional<NewspaperShown> newspaper_;

    // The debriefing picture drawn for the results page (game/Debrief.h).
    struct DebriefShown {
        bool completed = false;
        bool survivalBonus = false;
        int specialMedalsOffered = 0;
        int specialMedalsWon = 0;
        std::vector<game::DebriefRow> rows;
        double seconds = 0.0;  // on screen, to turn the medals
    };
    std::optional<DebriefShown> debrief_;
    std::map<std::string, std::unique_ptr<render::UiImage>> debriefPictures_;
    // The medal of the debriefing page: the promotion point's ribbon and
    // medal textures (Chars/propoint.mtd), null if missing.
    std::unique_ptr<render::UiImage> medalRibbon_;
    std::unique_ptr<render::UiImage> medalFace_;
    bool medalLoaded_ = false;

    // The mission being played.
    struct PlayedMission {
        int number = 0;
        int playerTeam = -1;
        std::shared_ptr<std::size_t> deployed;  // filled in when the map's hogs are set up
    };
    std::optional<PlayedMission> played_;
};

} // namespace openhogs::gameapp

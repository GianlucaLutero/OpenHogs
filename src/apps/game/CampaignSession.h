// CampaignSession.h
//
// A campaign being played: the game::Campaign, its pages between missions
// (ui::CampaignMenu), the mission briefing, and the link to the matches:
//
//     squad page ── START MISSION ──> map ──> briefing ── Enter ──> mission (a Game)
//         ▲                                                   │
//         └──────────── mission results <─────────────────────┘
//
// Saves: the original's eight numbered slots (LOAD GAME, SAVE TEAM), as
// saves/army1.ini ... army8.ini. A new campaign takes the first free slot
// when its team is named, and the campaign is saved to its slot after
// every change (promotion, swap, mission), so quitting never loses
// progress; SAVE TEAM copies it to another slot, which becomes its own.
// A saves/campaign.ini from before the slots becomes slot 1.
//
// The campaign map (ui/CampaignMap.h) comes before every briefing but the
// Boot Camp's: the territories in their holders' colours, the island and
// its flags, from Language/Tims/PigMap/.
//
// Briefings are the game's pictures Language/Tims/Briefing/level<N>.bmp
// (translated by the language packs); mission 1 also shows a photo of a
// hog in the player's uniform, level1n<nation>.bmp, in its frame.

#pragma once

#include "Game.h"
#include "Interface.h"
#include "MenuModels.h"
#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Campaign.h"
#include "openhogs/game/Debrief.h"
#include "openhogs/formats/BitmapImage.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/CampaignMap.h"
#include "openhogs/ui/CampaignMenu.h"

#include <array>
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

    // `savePath`: saves/campaign.ini; the slots go next to it.
    static bool hasSave(const std::filesystem::path& savePath);

    // Starts a new campaign: PLEASE NAME YOUR TEAM, then the squad page.
    void startNew(int nation);
    // LOAD GAME: the loading board; picking a slot opens its squad page.
    void openLoad();

    // A key typed while a name is being written (NameEntry): letters and
    // Backspace. Returns false when no name is being written.
    bool typingName() const { return menu_->typingName(); }
    // The campaign being played (none on the loading board).
    const game::Campaign* campaign() const { return campaign_ ? &*campaign_ : nullptr; }
    void type(char letter, SoundPlayer& sounds);
    void erase(SoundPlayer& sounds);

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
    // The 3D models over the page just drawn: the debriefing's medals.
    void drawModels(MenuModels& models, const MenuModels::Screen& screen);
    // Counts down the newspaper's time on screen.
    void update(double seconds);
    // A full-screen picture (the briefing or the newspaper) hides the menus.
    bool showingPicture() const {
        return briefing_ != nullptr || newspaper_.has_value() || showingDebrief() || campaignMap_.has_value();
    }
    // The campaign's pages (for the original screens), if a campaign is open.
    const ui::CampaignMenu* menu() const { return menu_.get(); }
    // How many hogs the squad has.
    std::size_t squadSize() const { return campaign_ ? campaign_->squad().size() : 0; }

private:
    Action handleMenuInput(ui::MenuInput input, SoundPlayer& sounds);
    void save() const;
    std::filesystem::path slotPath(std::size_t slot) const;
    std::vector<ui::SaveSlot> readSlots() const;
    bool loadSlot(std::size_t slot);
    void saveToSlot(std::size_t slot);
    std::optional<std::size_t> firstFreeSlot() const;
    void teamNamed();
    // The next mission's briefing, with the campaign map before it.
    void openBriefing();
    void drawCampaignMap();
    bool loadCampaignMapPictures();
    bool showingDebrief() const;
    void openDebrief(const std::vector<game::SquadMember>& squad, const game::MissionReport& report,
                     const game::MissionOutcome& outcome, std::size_t deployed);
    void drawDebrief();
    void addMedal(float x, float y, bool won);
    // A picture of Language/Tims/Debrief/, loaded once (nullptr if missing).
    const render::UiImage* debriefPicture(const std::string& file, formats::BitmapTransparency transparency);
    void openNewspaper(const game::MissionReport& report, const game::MissionOutcome& outcome, int missionNumber);
    std::size_t deploymentFor(const game::Campaign& campaign);

    const core::VirtualFileSystem& vfs_;
    Interface& interface_;
    std::filesystem::path savePath_;
    game::VersionRules rules_;
    std::optional<game::Campaign> campaign_;
    std::optional<std::size_t> slot_;  // where the campaign is saved
    bool promotionHintGiven_ = false;  // the sergeant's squad-page line, once a session
    void givePromotionHint(SoundPlayer& sounds);
    std::unique_ptr<ui::CampaignMenu> menu_;
    std::map<std::string, std::size_t> slotsByMap_;  // places for the player's team, per map

    // The campaign map before the briefing, and its pictures (loaded once).
    std::optional<ui::CampaignMapAnimation> campaignMap_;
    struct CampaignMapPictures {
        std::unique_ptr<render::UiImage> map;                      // BigMap.bmp
        std::vector<render::UiImage> pieces;                       // PMAPTIMS.MAD: territories, arrows, flag
        std::vector<render::UiImage> names;                        // LANGTIMS.MAD: the islands' names
        std::array<std::unique_ptr<render::UiImage>, ui::kMapIslands> islands;  // hogphy.mad ...
    };
    std::optional<CampaignMapPictures> mapPictures_;
    bool mapPicturesMissing_ = false;

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
    // The debriefing page's medals, gathered while drawing the page and
    // drawn in 3D after it (drawModels).
    struct MedalShown {
        glm::vec3 position;  // the executable's scene: x right, y up, depth
        float opacity;
    };
    std::vector<MedalShown> medals_;

    // The mission being played.
    struct PlayedMission {
        int number = 0;
        int playerTeam = -1;
        std::shared_ptr<std::size_t> deployed;  // filled in when the map's hogs are set up
    };
    std::optional<PlayedMission> played_;
};

} // namespace openhogs::gameapp

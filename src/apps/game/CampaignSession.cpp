// CampaignSession.cpp - see CampaignSession.h for an overview.

#include "CampaignSession.h"

#include "openhogs/game/Cutscenes.h"
#include "openhogs/game/MatchSounds.h"
#include "openhogs/game/Newspaper.h"

#include "MenuControls.h"

#include "openhogs/core/IniFile.h"
#include "openhogs/formats/BitmapImage.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// Where mission 1's photo goes in its 640 x 480 briefing picture (the
// empty frame in level1.bmp, measured on the PC files).
constexpr float kPhotoX = 350.0f;
constexpr float kPhotoY = 197.0f;
constexpr float kBriefingWidth = 640.0f;  // also the newspapers' width

// The photos level1n0..5 show the uniforms in the order British,
// American, French, German, Russian, Japanese (checked against the team
// textures); scene::characterTeams has Japanese before Russian.
constexpr std::array<int, game::kPlayableNations> kPhotoOfNation{0, 1, 2, 3, 5, 4};

std::unique_ptr<render::UiImage> loadPicture(const core::VirtualFileSystem& vfs, const std::string& path,
                                             formats::BitmapTransparency transparency = formats::BitmapTransparency::None) {
    try {
        return std::make_unique<render::UiImage>(formats::decodeBitmap(vfs.readFile(path), transparency));
    } catch (const std::exception& error) {
        std::cerr << "Warning: " << path << ": " << error.what() << "\n";
        return nullptr;
    }
}

// The debriefing page (Language/Tims/Debrief/, 640 x 480), in its pixels.
// The text positions are the executable's (FUN_004845e0); the pictures'
// are measured on the frames of pigbkpc1.bmp, as the executable keeps
// them in its picture objects.
const std::string kDebriefFolder = "Language/Tims/Debrief/";
constexpr float kRowStep = 73.0f;           // between two hogs
constexpr float kFaceX = 83.0f;             // the frame's inside: 84..151 x 71..130
constexpr float kFaceY = 71.0f;
constexpr float kFrameWidth = 68.0f;
constexpr float kFrameHeight = 60.0f;
constexpr float kNameX = 215.0f;            // centre of the name plate
constexpr float kNameY = 90.0f;
constexpr float kBadgeY = 110.0f;           // in the plate's lower tab
constexpr float kBadgeSpread = 14.0f;       // career badge and pips side by side
constexpr float kTitleX = 320.0f;
constexpr float kTitleY = 5.0f;
constexpr float kPanelX = 456.0f;           // centre of the medals panel
constexpr float kResultsY = 84.0f;
constexpr float kFailedY = 226.0f;
constexpr float kLevelCompleteY = 140.0f;
constexpr float kSurvivalY = 225.0f;
constexpr float kNoneY = 260.0f;
constexpr float kSpecialY = 305.0f;
// The medals are 3D models in the executable, placed at y = 700, -500 and
// -1700 (in a row 750 apart) at depth 7000; on the page they land about
// here (the medal's centre).
constexpr float kLevelMedalY = 190.0f;
constexpr float kSurvivalMedalY = 273.0f;
constexpr float kSpecialMedalY = 355.0f;
constexpr float kSpecialMedalStep = 50.0f;
constexpr float kMedalScale = 0.6f;
// The executable turns them 25/4096 of a turn every 5 ms.
constexpr double kMedalTurnsPerSecond = 25.0 / 4096.0 * 200.0;
constexpr float kMedalNotWonOpacity = 100.0f / 255.0f;

std::unique_ptr<render::UiImage> loadTimFromArchive(const core::VirtualFileSystem& vfs, const std::string& archive,
                                                    std::string_view name) {
    try {
        const auto package = formats::PackageArchive::parse(vfs.readFile(archive));
        const auto* entry = package.find(name);
        if (!entry) return nullptr;
        return std::make_unique<render::UiImage>(formats::decodeTim(package.entryData(*entry)));
    } catch (const std::exception& error) {
        std::cerr << "Warning: " << archive << ": " << error.what() << "\n";
        return nullptr;
    }
}

} // namespace

CampaignSession::CampaignSession(const core::VirtualFileSystem& vfs, Interface& userInterface,
                                 std::filesystem::path savePath, game::VersionRules rules)
    : vfs_(vfs), interface_(userInterface), savePath_(std::move(savePath)), rules_(rules) {
    menu_ = std::make_unique<ui::CampaignMenu>(
        interface_.strings(), [this](const game::Campaign& campaign) { return deploymentFor(campaign); });
}

namespace {

constexpr std::size_t kSlots = ui::kSaveSlots;

std::filesystem::path slotFile(const std::filesystem::path& savePath, std::size_t slot) {
    return savePath.parent_path() / ("army" + std::to_string(slot + 1) + ".ini");
}

// A campaign.ini from before the slots becomes slot 1 (when that is free).
void moveOldSave(const std::filesystem::path& savePath) {
    std::error_code error;
    if (!std::filesystem::exists(savePath, error) || std::filesystem::exists(slotFile(savePath, 0), error)) return;
    std::filesystem::rename(savePath, slotFile(savePath, 0), error);
    if (error) std::cerr << "Warning: cannot move " << savePath.string() << " to slot 1: " << error.message() << "\n";
}

} // namespace

bool CampaignSession::hasSave(const std::filesystem::path& savePath) {
    moveOldSave(savePath);
    std::error_code error;
    for (std::size_t slot = 0; slot < kSlots; ++slot) {
        if (std::filesystem::exists(slotFile(savePath, slot), error)) return true;
    }
    return false;
}

std::filesystem::path CampaignSession::slotPath(std::size_t slot) const { return slotFile(savePath_, slot); }

std::vector<ui::SaveSlot> CampaignSession::readSlots() const {
    std::vector<ui::SaveSlot> slots(kSlots);
    for (std::size_t i = 0; i < kSlots; ++i) {
        std::error_code error;
        if (!std::filesystem::exists(slotPath(i), error)) continue;
        try {
            const auto campaign = game::loadCampaign(core::IniFile::load(slotPath(i)));
            auto& slot = slots[i];
            slot.used = true;
            slot.name = campaign.teamName().empty() ? interface_.strings().teamName(campaign.nation()) : campaign.teamName();
            slot.nextMission = campaign.nextMission();
            for (const auto& member : campaign.squad()) slot.squad.push_back(member.rank);
        } catch (const std::exception& problem) {
            std::cerr << "Warning: " << slotPath(i).string() << ": " << problem.what() << "\n";
        }
    }
    return slots;
}

std::optional<std::size_t> CampaignSession::firstFreeSlot() const {
    const auto slots = readSlots();
    for (std::size_t i = 0; i < slots.size(); ++i) {
        std::error_code error;
        if (!slots[i].used && !std::filesystem::exists(slotPath(i), error)) return i;
    }
    return std::nullopt;
}

std::size_t CampaignSession::deploymentFor(const game::Campaign& campaign) {
    if (campaign.finished()) {
        return campaign.deploymentSize(game::kDeployedHogs);
    }
    const std::string map(game::campaignMission(campaign.nextMission()).map);
    auto found = slotsByMap_.find(map);
    if (found == slotsByMap_.end()) {
        std::size_t slots = game::kDeployedHogs;
        try {
            game::loadWorld(vfs_, map, {}, [&](std::vector<game::Hog>& hogs) { slots = game::playerSlots(hogs); });
        } catch (const std::exception& error) {
            std::cerr << "Warning: " << map << ": " << error.what() << "\n";
        }
        found = slotsByMap_.emplace(map, slots).first;
    }
    return campaign.deploymentSize(found->second);
}

void CampaignSession::save() const {
    if (!campaign_ || !slot_) return;
    const auto path = slotPath(*slot_);
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path);
    file << campaign_->save();
    if (!file) {
        std::cerr << "Warning: cannot save the campaign to " << path.string() << "\n";
    }
}

void CampaignSession::saveToSlot(std::size_t slot) {
    slot_ = slot;
    save();
    menu_->setSlots(readSlots());
}

void CampaignSession::startNew(int nation) {
    campaign_.emplace(nation, interface_.strings().hogNames.at(static_cast<std::size_t>(nation)));
    slot_.reset();  // a slot once the team has a name
    briefing_.reset();
    debrief_.reset();
    newspaper_.reset();
    menu_->showNameTeam(*campaign_);
}

void CampaignSession::openLoad() {
    moveOldSave(savePath_);
    campaign_.reset();
    slot_.reset();
    briefing_.reset();
    debrief_.reset();
    newspaper_.reset();
    menu_->showLoad(readSlots());
}

bool CampaignSession::loadSlot(std::size_t slot) {
    try {
        campaign_ = game::loadCampaign(core::IniFile::load(slotPath(slot)));
    } catch (const std::exception& error) {
        std::cerr << "Error: cannot load the campaign " << slotPath(slot).string() << ": " << error.what() << "\n";
        return false;
    }
    slot_ = slot;
    menu_->setSlots(readSlots());
    menu_->showSquad(*campaign_);
    return true;
}

void CampaignSession::type(char letter, SoundPlayer& sounds) {
    if (!campaign_) return;
    playMenuSound(sounds, menu_->type(letter, *campaign_).sound);
}

void CampaignSession::teamNamed() {
    // A new campaign keeps itself in the first free slot, if there is one.
    if (!slot_) slot_ = firstFreeSlot();
    save();
    menu_->setSlots(readSlots());
}

void CampaignSession::erase(SoundPlayer& sounds) { playMenuSound(sounds, menu_->erase().sound); }

void CampaignSession::openBriefing() {
    const int number = campaign_->nextMission();
    briefing_ = loadPicture(vfs_, "Language/Tims/Briefing/level" + std::to_string(number) + ".bmp");
    briefingPhoto_.reset();
    if (number == 1) {
        briefingPhoto_ = loadPicture(vfs_, "Language/Tims/Briefing/level1n" +
                                                 std::to_string(kPhotoOfNation.at(static_cast<std::size_t>(campaign_->nation()))) + ".bmp");
    }
}

void CampaignSession::openNewspaper(const game::MissionReport& report, const game::MissionOutcome& outcome,
                                    int missionNumber) {
    game::NewspaperFacts facts;
    facts.nation = campaign_->nation();
    facts.missionNumber = missionNumber;
    facts.hogsLost = static_cast<int>(report.deathOrder.size());
    facts.medals = (outcome.survivalBonus ? 1 : 0) + (outcome.islandConquered ? 1 : 0) +
                   std::max(0, outcome.specialMedalsWon);
    const auto page = game::newspaperFor(facts);

    NewspaperShown shown;
    shown.paper = loadPicture(vfs_, page.paper);
    if (!shown.paper) return;  // no paper, no newspaper: straight to the results
    shown.story = loadPicture(vfs_, page.story, formats::BitmapTransparency::Red);
    shown.picture = loadPicture(vfs_, page.picture, formats::BitmapTransparency::Red);
    shown.secondsLeft = game::kNewspaperSeconds;
    newspaper_ = std::move(shown);
}

bool CampaignSession::showingDebrief() const {
    return debrief_.has_value() && menu_->screen() == ui::CampaignScreen::Results && !briefing_;
}

void CampaignSession::openDebrief(const std::vector<game::SquadMember>& squad, const game::MissionReport& report,
                                  const game::MissionOutcome& outcome, std::size_t deployed) {
    DebriefShown shown;
    shown.completed = outcome.completed;
    shown.survivalBonus = outcome.survivalBonus;
    shown.specialMedalsOffered = outcome.specialMedalsOffered;
    shown.specialMedalsWon = std::max(0, outcome.specialMedalsWon);
    shown.rows = game::debriefRows(squad, report, deployed, outcome.completed);
    debrief_ = std::move(shown);
}

const render::UiImage* CampaignSession::debriefPicture(const std::string& file, formats::BitmapTransparency transparency) {
    auto found = debriefPictures_.find(file);
    if (found == debriefPictures_.end()) {
        found = debriefPictures_.emplace(file, loadPicture(vfs_, kDebriefFolder + file, transparency)).first;
    }
    return found->second.get();
}

void CampaignSession::update(double seconds) {
    if (debrief_) debrief_->seconds += seconds;
    if (!newspaper_) return;
    newspaper_->secondsLeft -= seconds;
    if (newspaper_->secondsLeft <= 0.0) newspaper_.reset();
}

void CampaignSession::givePromotionHint(SoundPlayer& sounds) {
    if (promotionHintGiven_ || !campaign_ || menu_->screen() != ui::CampaignScreen::Squad) return;
    if (!game::wantsPromotionHint(campaign_->nextMission(), campaign_->points())) return;
    promotionHintGiven_ = true;
    menu_->pointAtFirstHog();
    sounds.say(game::promotionHintPath());
}

CampaignSession::Action CampaignSession::handleInput(ui::MenuInput input, SoundPlayer& sounds) {
    const Action action = handleMenuInput(input, sounds);
    if (action == Action::None) givePromotionHint(sounds);
    return action;
}

CampaignSession::Action CampaignSession::handleMenuInput(ui::MenuInput input, SoundPlayer& sounds) {
    if (menu_->screen() == ui::CampaignScreen::Load) {
        const auto result = menu_->handleLoad(input);
        playMenuSound(sounds, result.sound);
        if (result.request == ui::CampaignMenuResult::Request::Leave) return Action::Leave;
        if (result.request == ui::CampaignMenuResult::Request::LoadFromSlot && !loadSlot(result.slot)) {
            playMenuSound(sounds, ui::MenuSound::Back);
        }
        return Action::None;
    }
    if (!campaign_) return Action::Leave;

    if (newspaper_) {
        // Any key puts the paper down, as in the original.
        newspaper_.reset();
        return Action::None;
    }

    if (showingDebrief()) {
        const auto result = menu_->answerDebrief(input, *campaign_);
        playMenuSound(sounds, result.sound);
        if (menu_->screen() != ui::CampaignScreen::Results) debrief_.reset();
        if (result.request == ui::CampaignMenuResult::Request::StartMission) {
            debrief_.reset();
            openBriefing();
            if (!briefing_) return Action::StartMission;
        } else if (result.request == ui::CampaignMenuResult::Request::Leave) {
            debrief_.reset();
            save();
            return Action::Leave;
        }
        return Action::None;
    }

    if (briefing_) {
        // Enter (or Space) goes to the mission, Esc back to the squad.
        if (input == ui::MenuInput::Confirm) {
            playMenuSound(sounds, ui::MenuSound::Select);
            briefing_.reset();
            return Action::StartMission;
        }
        if (input == ui::MenuInput::Back) {
            playMenuSound(sounds, ui::MenuSound::Back);
            briefing_.reset();
        }
        return Action::None;
    }

    const bool namingTeam = menu_->screen() == ui::CampaignScreen::NameTeam;
    const auto result = menu_->handle(input, *campaign_);
    playMenuSound(sounds, result.sound);
    switch (result.request) {
    case ui::CampaignMenuResult::Request::Changed:
        if (namingTeam) {
            teamNamed();
        } else {
            save();
        }
        break;
    case ui::CampaignMenuResult::Request::SaveToSlot:
        saveToSlot(result.slot);
        break;
    case ui::CampaignMenuResult::Request::LoadFromSlot:
        loadSlot(result.slot);
        break;
    case ui::CampaignMenuResult::Request::StartMission:
        openBriefing();
        if (!briefing_) {
            return Action::StartMission;  // no picture: straight to the mission
        }
        break;
    case ui::CampaignMenuResult::Request::Leave:
        if (namingTeam) {
            campaign_.reset();  // the unnamed team is dropped
        } else {
            save();
        }
        return Action::Leave;
    case ui::CampaignMenuResult::Request::None:
        break;
    }
    return Action::None;
}

GameOptions CampaignSession::missionOptions() {
    const game::Mission& mission = game::campaignMission(campaign_->nextMission());
    GameOptions options;
    options.mapName = std::string(mission.map);
    options.turnSeconds = mission.turnSeconds;

    // The map's teams, the player's first: the player fights for the
    // campaign's nation, the others for this mission's enemies.
    const auto teams = game::mapTeams(vfs_, mission.map);
    PlayedMission played;
    played.number = mission.number;
    played.playerTeam = teams.empty() ? -1 : teams.front();
    played.deployed = std::make_shared<std::size_t>(0);
    for (std::size_t i = 0; i < teams.size(); ++i) {
        options.teamNations[teams[i]] = i == 0 ? campaign_->nation() : campaign_->enemyNation(mission.number, i - 1);
        if (i > 0) options.computerTeams.push_back(teams[i]);
    }

    // The squad takes the places of the map's player hogs.
    const game::Campaign campaign = *campaign_;
    const auto deployed = played.deployed;
    options.prepareHogs = [campaign, deployed](std::vector<game::Hog>& hogs) {
        *deployed = campaign.deploymentSize(game::playerSlots(hogs));
        game::deploySquad(hogs, campaign, *deployed);
    };
    played_ = played;
    return options;
}

std::vector<std::string> CampaignSession::missionEnded(const Game& game) {
    if (!campaign_ || !played_) return {};
    std::vector<std::string> videos;
    const auto& world = game.world();
    ui::MissionSummary summary;
    summary.missionNumber = played_->number;
    summary.aborted = world.phase() != game::MatchPhase::Over;
    debrief_.reset();
    if (!summary.aborted) {
        game::MissionReport report;
        report.won = world.winner() == played_->playerTeam;
        report.deathOrder = game.squadDeaths();
        report.pickedUpPoints = game.promotionPoints();
        // The debriefing shows the squad as it fought, before the hogs
        // lost for good are replaced.
        const auto squadBefore = campaign_->squad();
        summary.outcome = campaign_->recordMission(report, *played_->deployed);
        openDebrief(squadBefore, report, summary.outcome, *played_->deployed);
        if (report.won) {
            // The PlayStation: the first victory leads into the hard mode,
            // whose own ending (FMV 09) waits for the second.
            const bool hardModeNext = rules_.hardMode && campaign_->finished() && !campaign_->hardMode();
            videos = game::campaignCutscenes(campaign_->nextMission(), !hardModeNext);
            if (hardModeNext) campaign_->startHardMode();
            // Every victory makes the papers, except the Boot Camp's.
            if (summary.outcome.completed && summary.missionNumber != game::kBootCampMission) {
                openNewspaper(report, summary.outcome, summary.missionNumber);
            }
        }
        save();
    }
    played_.reset();
    menu_->showResults(*campaign_, summary);
    return videos;
}

void CampaignSession::draw() {
    auto& ui = interface_.renderer();
    if (showingDebrief() && !newspaper_) {
        drawDebrief();
        return;
    }
    if (newspaper_) {
        // The paper is drawn 480 units tall, so its pixels are canvas
        // units; the story and the photo keep their place on it.
        interface_.drawPicture(*newspaper_->paper);
        const float left = (ui.canvasWidth() - kBriefingWidth) / 2.0f;
        const auto lay = [&](const std::unique_ptr<render::UiImage>& image, int x, int y) {
            if (image) ui.drawImage(*image, left + static_cast<float>(x), static_cast<float>(y), image->width(), image->height());
        };
        lay(newspaper_->story, game::kNewspaperStoryX, game::kNewspaperStoryY);
        lay(newspaper_->picture, game::kNewspaperPictureX, game::kNewspaperPictureY);
        return;
    }
    if (briefing_) {
        interface_.drawPicture(*briefing_);
        if (briefingPhoto_) {
            // The briefing picture is drawn 480 units tall, so its pixels
            // are canvas units; it is centred horizontally.
            const float left = (ui.canvasWidth() - kBriefingWidth) / 2.0f;
            ui.drawImage(*briefingPhoto_, left + kPhotoX, kPhotoY, briefingPhoto_->width(), briefingPhoto_->height());
        }
        return;
    }
    interface_.drawMenu(menu_->menu(), 10, menu_->lines());
}

void CampaignSession::drawMedal(float x, float y, bool won) {
    if (!medalLoaded_) {
        medalLoaded_ = true;
        medalRibbon_ = loadTimFromArchive(vfs_, "Chars/propoint.mtd", "pp001.tim");
        medalFace_ = loadTimFromArchive(vfs_, "Chars/propoint.mtd", "pp002.tim");
    }
    if (!medalRibbon_ || !medalFace_) return;
    // The model turns about its upright axis: drawn flat, it narrows and
    // widens again (decision 161).
    const double turns = debrief_ ? debrief_->seconds * kMedalTurnsPerSecond : 0.0;
    const float narrowing = static_cast<float>(std::abs(std::cos(turns * 2.0 * 3.14159265358979)));
    const glm::vec4 tint(1.0f, 1.0f, 1.0f, won ? 1.0f : kMedalNotWonOpacity);
    auto& ui = interface_.renderer();
    const float ribbonWidth = medalRibbon_->width() * kMedalScale * narrowing;
    const float ribbonHeight = medalRibbon_->height() * kMedalScale;
    const float faceWidth = medalFace_->width() * kMedalScale * narrowing;
    const float faceHeight = medalFace_->height() * kMedalScale;
    const float top = y - (ribbonHeight + faceHeight) / 2.0f;
    ui.drawImage(*medalRibbon_, x - ribbonWidth / 2.0f, top, ribbonWidth, ribbonHeight, tint);
    ui.drawImage(*medalFace_, x - faceWidth / 2.0f, top + ribbonHeight, faceWidth, faceHeight, tint);
}

void CampaignSession::drawDebrief() {
    auto& ui = interface_.renderer();
    const auto& strings = interface_.strings();
    const auto& shown = *debrief_;
    using formats::BitmapTransparency;

    // The background: pigbkpc1 after a victory, pigbkpc2 (other keys) after a defeat.
    if (const auto* background = debriefPicture(shown.completed ? "Pigbkpc1.bmp" : "Pigbkpc2.bmp", BitmapTransparency::None)) {
        interface_.drawPicture(*background);
    }
    const float left = (ui.canvasWidth() - kBriefingWidth) / 2.0f;
    const auto place = [&](const render::UiImage* image, float x, float y) {
        if (image) ui.drawImage(*image, left + x, y, image->width(), image->height());
    };
    render::TextStyle centred;
    centred.align = render::TextAlign::Centre;

    // The squad: uniform and face in the frame, name and badges on the plate.
    static constexpr std::array<std::string_view, game::kPlayableNations> kUniforms{
        "unifeng.bmp", "unifusa.bmp", "uniffren.bmp", "unifgerm.bmp", "unifjap.bmp", "unifruss.bmp"};
    const auto nation = static_cast<std::size_t>(campaign_->nation());
    for (std::size_t i = 0; i < shown.rows.size(); ++i) {
        const auto& row = shown.rows[i];
        const float rowY = kRowStep * static_cast<float>(i);
        if (row.face == game::DebriefFace::Lost) {
            const auto* grave = debriefPicture("r_i_p.bmp", BitmapTransparency::Magenta);
            if (grave) {
                place(grave, kFaceX + 1.0f + (kFrameWidth - grave->width()) / 2.0f,
                      kFaceY + rowY + (kFrameHeight - grave->height()) / 2.0f);
            }
        } else {
            const auto* uniform = debriefPicture(std::string(kUniforms.at(nation)), BitmapTransparency::Magenta);
            if (uniform) {
                place(uniform, kFaceX + 1.0f + (kFrameWidth - uniform->width()) / 2.0f,
                      kFaceY + rowY + kFrameHeight - uniform->height());
            }
            const std::string face = (row.face == game::DebriefFace::Hurt ? "facepcw" : "Facepc") +
                                     std::to_string(row.faceNumber) + ".bmp";
            place(debriefPicture(face, BitmapTransparency::Magenta), kFaceX, kFaceY + rowY);
        }
        ui.drawText(interface_.smallFont(), row.name, left + kNameX, kNameY + rowY, centred);
        const auto badges = game::rankBadges(row.rank);
        for (std::size_t b = 0; b < badges.size(); ++b) {
            const auto* badge = debriefPicture(badges[b], BitmapTransparency::PaletteIndex0);
            if (!badge) continue;
            const float offset = badges.size() == 1 ? 0.0f : (b == 0 ? -kBadgeSpread : kBadgeSpread);
            place(badge, kNameX + offset - badge->width() / 2.0f, kBadgeY + rowY);
        }
    }

    // The mission, and its medals.
    if (!menu_->lines().empty()) {
        ui.drawText(interface_.bigFont(), menu_->lines().front(), left + kTitleX, kTitleY, centred);
    }
    const auto& font = interface_.smallFont();
    const float panel = left + kPanelX;
    ui.drawText(font, strings.missionResults, panel, kResultsY, centred);
    if (!shown.completed) {
        ui.drawText(font, strings.missionFailed, panel, kFailedY, centred);
        return;
    }
    ui.drawText(font, strings.levelComplete, panel, kLevelCompleteY, centred);
    drawMedal(panel, kLevelMedalY, true);
    ui.drawText(font, strings.survivalBonus, panel, kSurvivalY, centred);
    if (shown.survivalBonus) {
        drawMedal(panel, kSurvivalMedalY, true);
    } else {
        ui.drawText(font, strings.none, panel, kNoneY, centred);
    }
    if (shown.specialMedalsOffered > 0) {
        ui.drawText(font, strings.specialBonus, panel, kSpecialY, centred);
        // In a row, the medals not picked up faded.
        const float first = -kSpecialMedalStep * static_cast<float>(shown.specialMedalsOffered - 1) / 2.0f;
        for (int k = 0; k < shown.specialMedalsOffered; ++k) {
            drawMedal(panel + first + kSpecialMedalStep * static_cast<float>(k), kSpecialMedalY,
                      k < shown.specialMedalsWon);
        }
    }
}

} // namespace openhogs::gameapp

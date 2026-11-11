// CampaignSession.cpp - see CampaignSession.h for an overview.

#include "CampaignSession.h"

#include "MenuControls.h"

#include "openhogs/core/IniFile.h"
#include "openhogs/formats/BitmapImage.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// Where mission 1's photo goes in its 640 x 480 briefing picture (the
// empty frame in level1.bmp, measured on the PC files).
constexpr float kPhotoX = 350.0f;
constexpr float kPhotoY = 197.0f;
constexpr float kBriefingWidth = 640.0f;

// The photos level1n0..5 show the uniforms in the order British,
// American, French, German, Russian, Japanese (checked against the team
// textures); scene::characterTeams has Japanese before Russian.
constexpr std::array<int, game::kPlayableNations> kPhotoOfNation{0, 1, 2, 3, 5, 4};

std::unique_ptr<render::UiImage> loadPicture(const core::VirtualFileSystem& vfs, const std::string& path) {
    try {
        return std::make_unique<render::UiImage>(formats::decodeBitmap(vfs.readFile(path)));
    } catch (const std::exception& error) {
        std::cerr << "Warning: " << path << ": " << error.what() << "\n";
        return nullptr;
    }
}

} // namespace

CampaignSession::CampaignSession(const core::VirtualFileSystem& vfs, Interface& userInterface,
                                 std::filesystem::path savePath)
    : vfs_(vfs), interface_(userInterface), savePath_(std::move(savePath)) {
    menu_ = std::make_unique<ui::CampaignMenu>(
        interface_.strings(), [this](const game::Campaign& campaign) { return deploymentFor(campaign); });
}

bool CampaignSession::hasSave(const std::filesystem::path& savePath) {
    std::error_code error;
    return std::filesystem::exists(savePath, error);
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
    if (!campaign_) return;
    std::error_code error;
    std::filesystem::create_directories(savePath_.parent_path(), error);
    std::ofstream file(savePath_);
    file << campaign_->save();
    if (!file) {
        std::cerr << "Warning: cannot save the campaign to " << savePath_.string() << "\n";
    }
}

void CampaignSession::startNew(int nation) {
    campaign_.emplace(nation, interface_.strings().hogNames.at(static_cast<std::size_t>(nation)));
    save();
    briefing_.reset();
    menu_->showSquad(*campaign_);
}

bool CampaignSession::loadSaved() {
    try {
        campaign_ = game::loadCampaign(core::IniFile::load(savePath_));
    } catch (const std::exception& error) {
        std::cerr << "Error: cannot load the campaign " << savePath_.string() << ": " << error.what() << "\n";
        return false;
    }
    briefing_.reset();
    menu_->showSquad(*campaign_);
    return true;
}

void CampaignSession::openBriefing() {
    const int number = campaign_->nextMission();
    briefing_ = loadPicture(vfs_, "Language/Tims/Briefing/level" + std::to_string(number) + ".bmp");
    briefingPhoto_.reset();
    if (number == 1) {
        briefingPhoto_ = loadPicture(vfs_, "Language/Tims/Briefing/level1n" +
                                                 std::to_string(kPhotoOfNation.at(static_cast<std::size_t>(campaign_->nation()))) + ".bmp");
    }
}

CampaignSession::Action CampaignSession::handleInput(ui::MenuInput input, SoundPlayer& sounds) {
    if (!campaign_) return Action::Leave;

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

    const auto result = menu_->handle(input, *campaign_);
    playMenuSound(sounds, result.sound);
    switch (result.request) {
    case ui::CampaignMenuResult::Request::Changed:
        save();
        break;
    case ui::CampaignMenuResult::Request::StartMission:
        openBriefing();
        if (!briefing_) {
            return Action::StartMission;  // no picture: straight to the mission
        }
        break;
    case ui::CampaignMenuResult::Request::Leave:
        save();
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

void CampaignSession::missionEnded(const Game& game) {
    if (!campaign_ || !played_) return;
    const auto& world = game.world();
    ui::MissionSummary summary;
    summary.missionNumber = played_->number;
    summary.aborted = world.phase() != game::MatchPhase::Over;
    if (!summary.aborted) {
        game::MissionReport report;
        report.won = world.winner() == played_->playerTeam;
        report.deathOrder = game.squadDeaths();
        summary.outcome = campaign_->recordMission(report, *played_->deployed);
        save();
    }
    played_.reset();
    menu_->showResults(*campaign_, summary);
}

void CampaignSession::draw() {
    auto& ui = interface_.renderer();
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

} // namespace openhogs::gameapp

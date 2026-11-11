// FrontEnd.cpp - see FrontEnd.h for an overview.

#include "openhogs/ui/FrontEnd.h"

#include <utility>

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = FrontEndResult::Request;

// Item ids.
enum MainItem { kOnePlayer, kMultiPlayer, kOptions, kQuit };
enum OnePlayerItem { kNewGame, kLoadGame, kSkirmish, kOnePlayerBack };
constexpr int kTeamBack = -1;  // nation items use the nation index as id
enum OptionsItem { kMaster, kEffects, kVoices, kMusic, kOptionsBack };
enum ConfirmItem { kYes, kNo };
constexpr int kLevelBack = -1;  // map items use their index as id
// Players page: team items use their index in the team list as id.
constexpr int kStartGame = -1;
constexpr int kPlayersBack = -2;
constexpr int kHumanChoice = 0;
constexpr int kComputerChoice = 1;

MenuItem action(int id, std::string label, bool enabled = true) {
    MenuItem item;
    item.id = id;
    item.label = std::move(label);
    item.enabled = enabled;
    return item;
}

MenuItem slider(int id, std::string label, int value) {
    MenuItem item;
    item.id = id;
    item.label = std::move(label);
    item.kind = MenuItem::Kind::Slider;
    item.value = value;
    item.maxValue = kMaxVolumeLevel;
    return item;
}

// The sound of a plain menu response: moving, changing, going back.
MenuSound soundFor(const MenuResponse& response) {
    switch (response.type) {
    case Type::Moved:
    case Type::Changed: return MenuSound::Move;
    case Type::Activated: return MenuSound::Select;
    case Type::Back: return MenuSound::Back;
    case Type::None: break;
    }
    return MenuSound::None;
}

} // namespace

FrontEnd::FrontEnd(UiStrings strings, std::vector<std::string> maps, VolumeLevels volumes)
    : strings_(std::move(strings)), maps_(std::move(maps)), volumes_(volumes) {
    main_ = Menu(strings_.mainMenu, {action(kOnePlayer, strings_.onePlayer),
                                     action(kMultiPlayer, strings_.multiPlayer), action(kOptions, strings_.options),
                                     action(kQuit, strings_.quit)});

    onePlayerMenu_ = Menu(strings_.onePlayer, {action(kNewGame, strings_.newGame), action(kLoadGame, strings_.loadGame, false),
                                               action(kSkirmish, strings_.skirmish),
                                               action(kOnePlayerBack, strings_.back)});

    std::vector<MenuItem> nations;
    for (int nation = 0; nation < game::kPlayableNations; ++nation) {
        nations.push_back(action(nation, strings_.teamName(nation)));
    }
    nations.push_back(action(kTeamBack, strings_.back));
    selectTeam_ = Menu(strings_.selectTeam, std::move(nations));

    std::vector<MenuItem> levels;
    for (std::size_t i = 0; i < maps_.size(); ++i) {
        levels.push_back(action(static_cast<int>(i), maps_[i]));
    }
    levels.push_back(action(kLevelBack, strings_.back));
    selectLevel_ = Menu(strings_.selectLevel, std::move(levels));

    options_ = Menu(strings_.volumeControl,
                    {slider(kMaster, strings_.master, volumes_.master), slider(kEffects, strings_.effects, volumes_.effects),
                     slider(kVoices, strings_.voices, volumes_.speech), slider(kMusic, strings_.music, volumes_.music),
                     action(kOptionsBack, strings_.back)});

    // "No" is selected first, so a quick double Enter does not quit.
    confirmQuit_ = Menu(strings_.areYouSure, {action(kYes, strings_.yes), action(kNo, strings_.no)});
}

void FrontEnd::setSavedCampaign(bool exists) {
    // Rebuilt rather than edited: Menu items are read-only from outside.
    onePlayerMenu_ = Menu(strings_.onePlayer, {action(kNewGame, strings_.newGame), action(kLoadGame, strings_.loadGame, exists),
                                               action(kSkirmish, strings_.skirmish),
                                               action(kOnePlayerBack, strings_.back)});
    if (exists) onePlayerMenu_.select(kLoadGame);
}

const Menu& FrontEnd::menu() const {
    switch (screen_) {
    case FrontEndScreen::Main: return main_;
    case FrontEndScreen::OnePlayer: return onePlayerMenu_;
    case FrontEndScreen::SelectTeam: return selectTeam_;
    case FrontEndScreen::SelectLevel: return selectLevel_;
    case FrontEndScreen::Players: return players_;
    case FrontEndScreen::Options: return options_;
    case FrontEndScreen::ConfirmQuit: return confirmQuit_;
    }
    return main_;
}

void FrontEnd::open(FrontEndScreen screen) {
    screen_ = screen;
    if (screen == FrontEndScreen::ConfirmQuit) {
        confirmQuit_.select(kNo);
    }
}

void FrontEnd::showMainMenu() {
    open(FrontEndScreen::Main);
}

FrontEndResult FrontEnd::handle(MenuInput input) {
    switch (screen_) {
    case FrontEndScreen::Main: return handleMain(main_.handle(input));
    case FrontEndScreen::OnePlayer: return handleOnePlayer(onePlayerMenu_.handle(input));
    case FrontEndScreen::SelectTeam: return handleSelectTeam(selectTeam_.handle(input));
    case FrontEndScreen::SelectLevel: return handleSelectLevel(selectLevel_.handle(input));
    case FrontEndScreen::Players: return handlePlayers(players_.handle(input));
    case FrontEndScreen::Options: return handleOptions(options_.handle(input));
    case FrontEndScreen::ConfirmQuit: return handleConfirmQuit(confirmQuit_.handle(input));
    }
    return {};
}

FrontEndResult FrontEnd::handleMain(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back) {
        open(FrontEndScreen::ConfirmQuit);
    } else if (response.type == Type::Activated) {
        switch (response.itemId) {
        case kOnePlayer:
            open(FrontEndScreen::OnePlayer);
            break;
        case kMultiPlayer:
            onePlayer_ = false;
            selectLevel_.setTitle(strings_.selectLevel);
            open(FrontEndScreen::SelectLevel);
            break;
        case kOptions: open(FrontEndScreen::Options); break;
        case kQuit: open(FrontEndScreen::ConfirmQuit); break;
        default: break;
        }
    }
    return result;
}

FrontEndResult FrontEnd::handleOnePlayer(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kOnePlayerBack)) {
        open(FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        switch (response.itemId) {
        case kNewGame: open(FrontEndScreen::SelectTeam); break;
        case kLoadGame: result.request = Request::LoadCampaign; break;
        case kSkirmish:
            // The level list, titled after the mode.
            onePlayer_ = true;
            selectLevel_.setTitle(strings_.skirmish);
            open(FrontEndScreen::SelectLevel);
            break;
        default: break;
        }
    }
    return result;
}

FrontEndResult FrontEnd::handleSelectTeam(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kTeamBack)) {
        open(FrontEndScreen::OnePlayer);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        result.request = Request::NewCampaign;
        result.nation = response.itemId;
        open(FrontEndScreen::OnePlayer);  // where Back from the campaign will lead
    }
    return result;
}

FrontEndResult FrontEnd::handleSelectLevel(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kLevelBack)) {
        open(onePlayer_ ? FrontEndScreen::OnePlayer : FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        chosenMap_ = maps_.at(static_cast<std::size_t>(response.itemId));
        result.request = Request::LevelChosen;
        result.map = chosenMap_;
    }
    return result;
}

FrontEndResult FrontEnd::setLevelTeams(const std::vector<int>& teams) {
    FrontEndResult result;
    levelTeams_ = teams;
    if (teams.empty()) {
        open(FrontEndScreen::SelectLevel);  // nothing to play on this map
        return result;
    }
    if (onePlayer_) {
        // The first team is the player's, the others the computer's.
        result.request = Request::StartMatch;
        result.map = chosenMap_;
        result.computerTeams.assign(teams.begin() + 1, teams.end());
        return result;
    }
    // Every team starts as a human player; left/right hands it to the CPU.
    std::vector<MenuItem> items;
    for (std::size_t i = 0; i < teams.size(); ++i) {
        MenuItem item;
        item.id = static_cast<int>(i);
        item.label = strings_.teamName(teams[i]);
        item.kind = MenuItem::Kind::Choice;
        item.choices = {strings_.player, strings_.cpu};
        item.value = kHumanChoice;
        items.push_back(item);
    }
    items.push_back(action(kStartGame, strings_.startGame));
    items.push_back(action(kPlayersBack, strings_.back));
    players_ = Menu(strings_.multiPlayer, std::move(items));
    players_.select(kStartGame);
    open(FrontEndScreen::Players);
    return result;
}

FrontEndResult FrontEnd::handlePlayers(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kPlayersBack)) {
        open(FrontEndScreen::SelectLevel);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated && response.itemId == kStartGame) {
        result.request = Request::StartMatch;
        result.map = chosenMap_;
        for (std::size_t i = 0; i < levelTeams_.size(); ++i) {
            if (players_.item(static_cast<int>(i)).value == kComputerChoice) {
                result.computerTeams.push_back(levelTeams_[i]);
            }
        }
    }
    return result;
}

FrontEndResult FrontEnd::handleOptions(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kOptionsBack)) {
        open(FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Changed) {
        volumes_.master = options_.item(kMaster).value;
        volumes_.effects = options_.item(kEffects).value;
        volumes_.speech = options_.item(kVoices).value;
        volumes_.music = options_.item(kMusic).value;
        result.request = Request::VolumesChanged;
    }
    return result;
}

FrontEndResult FrontEnd::handleConfirmQuit(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Activated && response.itemId == kYes) {
        result.request = Request::Quit;
    } else if (response.type == Type::Back || response.type == Type::Activated) {
        open(FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    }
    return result;
}

} // namespace openhogs::ui

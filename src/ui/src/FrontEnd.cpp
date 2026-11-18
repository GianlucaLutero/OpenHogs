// FrontEnd.cpp - see FrontEnd.h for an overview.

#include "openhogs/ui/FrontEnd.h"

#include <array>
#include <string>
#include <utility>

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = FrontEndResult::Request;

// Item ids.
enum MainItem { kOnePlayer, kMultiPlayer, kOptions, kQuit };
enum OnePlayerItem { kNewGame, kLoadGame, kSkirmish, kOnePlayerBack };
constexpr int kTeamBack = -1;  // nation items use the nation index as id
enum OptionsItem { kAudio, kControls, kCredits, kOptionsBack };
enum VolumeItem { kMode, kMaster, kEffects, kVoices };
constexpr int kPageBack = -1;  // BACK on CONTROLS and CREDITS
constexpr int kVoicesOff = 0, kVoicesOn = 1;
enum ConfirmItem { kYes, kNo };
constexpr int kLevelBack = -1;  // map items use their index as id
// Players page: two items per team (its nation, who plays it), with ids
// from the team's place in the list.
constexpr int kStartGame = -1;
constexpr int kPlayersBack = -2;
constexpr int kFieldConditions = -3;
constexpr int kHumanChoice = 0;
constexpr int kComputerChoice = 1;
constexpr int kOffChoice = 2;
int nationItem(std::size_t team) { return static_cast<int>(team) * 2; }
int controlItem(std::size_t team) { return static_cast<int>(team) * 2 + 1; }

// Field conditions page. The values offered follow the original's menu
// (fetext 303-320): turn times 10 to 99 s, health half/normal/double,
// sudden death off or after 10, 20, 30 rounds.
enum FieldItem { kFieldHogs, kFieldTurnTime, kFieldHealth, kFieldSuddenDeath, kFieldBack };
constexpr int kMostHogsPerTeam = 8;
constexpr std::array<double, 6> kTurnTimes{10, 20, 30, 45, 60, 99};
constexpr int kDefaultTurnTime = 4;  // 60 s
constexpr std::array<float, 3> kHealthFactors{1.0f, 0.5f, 2.0f};
constexpr std::array<int, 4> kSuddenDeathRounds{0, 10, 20, 30};

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

FrontEnd::FrontEnd(UiStrings strings, std::vector<std::string> maps, VolumeLevels volumes, KeyBindings keyBindings)
    : strings_(std::move(strings)), maps_(std::move(maps)), volumes_(volumes), keyBindings_(std::move(keyBindings)) {
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

    // FIELD CONDITIONS: plain numbers need no translation.
    MenuItem hogs;
    hogs.id = kFieldHogs;
    hogs.label = strings_.pigs;
    hogs.kind = MenuItem::Kind::Choice;
    hogs.choices.push_back(strings_.normal);
    for (int count = 1; count <= kMostHogsPerTeam; ++count) hogs.choices.push_back(std::to_string(count));
    MenuItem turnTime;
    turnTime.id = kFieldTurnTime;
    turnTime.label = strings_.turnTime;
    turnTime.kind = MenuItem::Kind::Choice;
    for (const double seconds : kTurnTimes) turnTime.choices.push_back(std::to_string(static_cast<int>(seconds)));
    turnTime.value = kDefaultTurnTime;
    MenuItem health;
    health.id = kFieldHealth;
    health.label = strings_.health;
    health.kind = MenuItem::Kind::Choice;
    health.choices = {strings_.normal, strings_.half, strings_.doubled};
    MenuItem suddenDeath;
    suddenDeath.id = kFieldSuddenDeath;
    suddenDeath.label = strings_.suddenDeath;
    suddenDeath.kind = MenuItem::Kind::Choice;
    suddenDeath.choices = {strings_.off, "10", "20", "30"};
    fieldConditions_ = Menu(strings_.fieldConditions, {hogs, turnTime, health, suddenDeath, action(kFieldBack, strings_.back)});

    std::vector<MenuItem> levels;
    for (std::size_t i = 0; i < maps_.size(); ++i) {
        levels.push_back(action(static_cast<int>(i), maps_[i]));
    }
    levels.push_back(action(kLevelBack, strings_.back));
    selectLevel_ = Menu(strings_.selectLevel, std::move(levels));

    // OPTIONS as the original (screen 2).
    options_ = Menu(strings_.options, {action(kAudio, strings_.audio), action(kControls, strings_.controls),
                                       action(kCredits, strings_.credits), action(kOptionsBack, strings_.back)});

    buildControlsMenu();

    // CREDITS: the rows themselves are drawn by the view (UiStrings::creditRows).
    credits_ = Menu(strings_.credits, {action(kPageBack, strings_.back)});

    // VOLUME CONTROL as the original (screen 20): no BACK entry, Esc leaves.
    if (volumes_.speech > 0) speechWhenOn_ = volumes_.speech;
    MenuItem mode;
    mode.id = kMode;
    mode.label = strings_.mode;
    mode.kind = MenuItem::Kind::Choice;
    mode.choices.assign(strings_.soundModes.begin(), strings_.soundModes.end());
    mode.value = static_cast<int>(volumes_.mode);
    MenuItem voices;
    voices.id = kVoices;
    voices.label = strings_.voices;
    voices.kind = MenuItem::Kind::Choice;
    voices.choices = {strings_.off, strings_.on};
    voices.value = volumes_.speech > 0 ? kVoicesOn : kVoicesOff;
    volume_ = Menu(strings_.volumeControl, {mode, slider(kMaster, strings_.master, volumes_.master),
                                            slider(kEffects, strings_.effects, volumes_.effects), voices});

    // "No" is selected first, so a quick double Enter does not quit.
    confirmQuit_ = Menu(strings_.reallyQuit, {action(kYes, strings_.yes), action(kNo, strings_.no)});
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
    case FrontEndScreen::FieldConditions: return fieldConditions_;
    case FrontEndScreen::Options: return options_;
    case FrontEndScreen::Volume: return volume_;
    case FrontEndScreen::Controls: return controls_;
    case FrontEndScreen::Credits: return credits_;
    case FrontEndScreen::ConfirmQuit: return confirmQuit_;
    case FrontEndScreen::BattleResults: return battleResults_;
    }
    return main_;
}

const Menu* FrontEnd::menuUnder() const {
    // As the original: VOLUME CONTROL over OPTIONS, FIELD CONDITIONS on the
    // level panel.
    if (screen_ == FrontEndScreen::Volume) return &options_;
    if (screen_ == FrontEndScreen::FieldConditions) return &selectLevel_;
    return nullptr;
}

std::vector<int> FrontEnd::teamControls() const {
    std::vector<int> controls;
    if (screen_ != FrontEndScreen::Players && screen_ != FrontEndScreen::FieldConditions) return controls;
    for (std::size_t i = 0; i < levelTeams_.size(); ++i) {
        controls.push_back(players_.item(controlItem(i)).value);
    }
    return controls;
}

void FrontEnd::open(FrontEndScreen screen) {
    screen_ = screen;
    if (screen == FrontEndScreen::Controls) {
        controls_.select(0);
        waitingControl_.reset();
    }
    if (screen == FrontEndScreen::ConfirmQuit) {
        confirmQuit_.select(kNo);
    }
}

void FrontEnd::showMainMenu() {
    open(FrontEndScreen::Main);
}

void FrontEnd::showBattleResults(std::vector<std::string> lines) {
    resultLines_ = std::move(lines);
    battleResults_ = Menu(strings_.battleResults, {action(0, strings_.continueGame)});
    open(FrontEndScreen::BattleResults);
}

FrontEndResult FrontEnd::handle(MenuInput input) {
    switch (screen_) {
    case FrontEndScreen::Main: return handleMain(main_.handle(input));
    case FrontEndScreen::OnePlayer: return handleOnePlayer(onePlayerMenu_.handle(input));
    case FrontEndScreen::SelectTeam: return handleSelectTeam(selectTeam_.handle(input));
    case FrontEndScreen::SelectLevel: return handleSelectLevel(selectLevel_.handle(input));
    case FrontEndScreen::Players: return handlePlayers(players_.handle(input));
    case FrontEndScreen::FieldConditions: return handleFieldConditions(fieldConditions_.handle(input));
    case FrontEndScreen::Options: return handleOptions(options_.handle(input));
    case FrontEndScreen::Volume: return handleVolume(volume_.handle(input));
    case FrontEndScreen::Controls:
        if (waitingControl_) {
            // A pad's back button cancels the wait like Escape; nothing
            // else is a key to give.
            return input == MenuInput::Back ? keyPressed(kAlwaysPauseKey) : FrontEndResult{};
        }
        return handleControls(controls_.handle(input));
    case FrontEndScreen::Credits: return handleOptionPage(credits_.handle(input));
    case FrontEndScreen::ConfirmQuit: return handleConfirmQuit(confirmQuit_.handle(input));
    case FrontEndScreen::BattleResults: {
        const auto response = battleResults_.handle(input);
        FrontEndResult result;
        result.sound = soundFor(response);
        if (response.type == Type::Activated || response.type == Type::Back) open(FrontEndScreen::Main);
        return result;
    }
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
        result.settings.computerTeams.assign(teams.begin() + 1, teams.end());
        return result;
    }

    // Two rows per team: its nation, then who plays it. Every team starts
    // as a human player in its usual nation.
    const auto nationCount = static_cast<int>(strings_.teamNames.size());
    std::vector<MenuItem> items;
    for (std::size_t i = 0; i < teams.size(); ++i) {
        MenuItem nation;
        nation.id = nationItem(i);
        nation.label = strings_.teamLabel(i);
        nation.kind = MenuItem::Kind::Choice;
        nation.choices.assign(strings_.teamNames.begin(), strings_.teamNames.end());
        nation.value = ((teams[i] % nationCount) + nationCount) % nationCount;
        items.push_back(nation);

        MenuItem control;
        control.id = controlItem(i);
        control.label = "";
        control.kind = MenuItem::Kind::Choice;
        control.choices = {strings_.player, strings_.cpu, strings_.off};
        control.value = kHumanChoice;
        items.push_back(control);
    }
    items.push_back(action(kStartGame, strings_.startGame));
    items.push_back(action(kFieldConditions, strings_.fieldConditions));
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
    } else if (response.type == Type::Activated && response.itemId == kFieldConditions) {
        open(FrontEndScreen::FieldConditions);
    } else if (response.type == Type::Activated && response.itemId == kStartGame) {
        MatchSettings settings = fieldSettings();
        int playing = 0;
        for (std::size_t i = 0; i < levelTeams_.size(); ++i) {
            const int team = levelTeams_[i];
            settings.teamNations[team] = players_.item(nationItem(i)).value;
            switch (players_.item(controlItem(i)).value) {
            case kComputerChoice: settings.computerTeams.push_back(team); ++playing; break;
            case kOffChoice: settings.conditions.removedTeams.push_back(team); break;
            default: ++playing; break;
            }
        }
        if (playing < 2) {
            result.sound = MenuSound::Back;  // a match needs two teams
            return result;
        }
        result.request = Request::StartMatch;
        result.map = chosenMap_;
        result.settings = settings;
    }
    return result;
}

MatchSettings FrontEnd::fieldSettings() const {
    MatchSettings settings;
    const int hogs = fieldConditions_.item(kFieldHogs).value;  // 0 = as on the map
    settings.conditions.hogsPerTeam = hogs;
    settings.conditions.healthFactor = kHealthFactors.at(static_cast<std::size_t>(fieldConditions_.item(kFieldHealth).value));
    settings.turnSeconds = kTurnTimes.at(static_cast<std::size_t>(fieldConditions_.item(kFieldTurnTime).value));
    settings.suddenDeathRound =
        kSuddenDeathRounds.at(static_cast<std::size_t>(fieldConditions_.item(kFieldSuddenDeath).value));
    return settings;
}

FrontEndResult FrontEnd::handleFieldConditions(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kFieldBack)) {
        open(FrontEndScreen::Players);
        result.sound = MenuSound::Back;
    }
    return result;
}

FrontEndResult FrontEnd::handleOptions(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kOptionsBack)) {
        open(FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        if (response.itemId == kAudio) open(FrontEndScreen::Volume);
        if (response.itemId == kControls) open(FrontEndScreen::Controls);
        if (response.itemId == kCredits) open(FrontEndScreen::Credits);
    }
    return result;
}

FrontEndResult FrontEnd::handleOptionPage(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kPageBack)) {
        open(FrontEndScreen::Options);
        result.sound = MenuSound::Back;
    }
    return result;
}

void FrontEnd::buildControlsMenu() {
    // CONTROLS: what each key does, with the original's names of the
    // actions (fetext 349-360) where it has one.
    const auto& names = strings_.controlNames;
    const auto label = [&](Control control) -> const std::string& {
        switch (control) {
        case Control::Forward: return names[0];
        case Control::Backward: return names[1];
        case Control::TurnLeft: return names[2];
        case Control::TurnRight: return names[3];
        case Control::Fire: return names[5];
        case Control::Jump: return names[6];
        case Control::PreviousWeapon: return strings_.previousWeapon;
        case Control::NextWeapon: return strings_.nextWeapon;
        case Control::Sight: return names[8];
        case Control::AimDown: return names[9];
        case Control::AimUp: return names[10];
        case Control::NextHog: return strings_.nextHog;
        case Control::EndTurn: return strings_.endTurn;
        case Control::Pause: return names[11];
        }
        return names[0];
    };
    std::vector<MenuItem> items;
    for (std::size_t i = 0; i < kControlCount; ++i) {
        const auto control = static_cast<Control>(i);
        MenuItem item = action(static_cast<int>(i), label(control));
        item.detail = keyBindings_.describe(control);
        items.push_back(item);
    }
    items.push_back(action(kPageBack, strings_.back));
    // Rebuilt after every change of key: the selection stays on its row.
    const int selectedId = controls_.items().empty() ? 0 : controls_.items()[controls_.selectedIndex()].id;
    controls_ = Menu(strings_.controls, std::move(items));
    controls_.select(selectedId);
}

FrontEndResult FrontEnd::handleControls(const MenuResponse& response) {
    if (response.type == Type::Activated && response.itemId >= 0 &&
        response.itemId < static_cast<int>(kControlCount)) {
        waitingControl_ = static_cast<Control>(response.itemId);
        FrontEndResult result;
        result.sound = MenuSound::Select;
        return result;
    }
    return handleOptionPage(response);
}

FrontEndResult FrontEnd::keyPressed(std::string_view key) {
    FrontEndResult result;
    if (!waitingControl_) return result;
    const Control control = *waitingControl_;
    waitingControl_.reset();
    if (!keyBindings_.bind(control, key)) {
        result.sound = MenuSound::Back;  // Escape: the keys stay as they were
        return result;
    }
    buildControlsMenu();
    result.sound = MenuSound::Select;
    result.request = FrontEndResult::Request::ControlsChanged;
    return result;
}

FrontEndResult FrontEnd::handleVolume(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back) {
        open(FrontEndScreen::Options);
    } else if (response.type == Type::Changed) {
        volumes_.mode = static_cast<SoundModeChoice>(volume_.item(kMode).value);
        volumes_.master = volume_.item(kMaster).value;
        volumes_.effects = volume_.item(kEffects).value;
        volumes_.speech = volume_.item(kVoices).value == kVoicesOn ? speechWhenOn_ : 0;
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

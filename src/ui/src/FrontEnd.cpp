// FrontEnd.cpp - see FrontEnd.h for an overview.

#include "openhogs/ui/FrontEnd.h"

#include <utility>

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = FrontEndResult::Request;

// Item ids.
enum MainItem { kOnePlayer, kMultiPlayer, kOptions, kQuit };
enum OptionsItem { kMaster, kEffects, kVoices, kMusic, kOptionsBack };
enum ConfirmItem { kYes, kNo };
constexpr int kLevelBack = -1;  // map items use their index as id

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
    // The campaign comes in a later block: ONE PLAYER is there, greyed out.
    main_ = Menu(strings_.mainMenu, {action(kOnePlayer, strings_.onePlayer, false),
                                     action(kMultiPlayer, strings_.multiPlayer), action(kOptions, strings_.options),
                                     action(kQuit, strings_.quit)});

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

const Menu& FrontEnd::menu() const {
    switch (screen_) {
    case FrontEndScreen::Main: return main_;
    case FrontEndScreen::SelectLevel: return selectLevel_;
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
    case FrontEndScreen::SelectLevel: return handleSelectLevel(selectLevel_.handle(input));
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
        case kMultiPlayer: open(FrontEndScreen::SelectLevel); break;
        case kOptions: open(FrontEndScreen::Options); break;
        case kQuit: open(FrontEndScreen::ConfirmQuit); break;
        default: break;
        }
    }
    return result;
}

FrontEndResult FrontEnd::handleSelectLevel(const MenuResponse& response) {
    FrontEndResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kLevelBack)) {
        open(FrontEndScreen::Main);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        result.request = Request::StartMatch;
        result.map = maps_.at(static_cast<std::size_t>(response.itemId));
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

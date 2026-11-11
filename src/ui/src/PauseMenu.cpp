// PauseMenu.cpp - see PauseMenu.h for an overview.

#include "openhogs/ui/PauseMenu.h"

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = PauseResult::Request;

enum PauseItem { kContinue, kAbort };
enum ConfirmItem { kYes, kNo };

MenuItem action(int id, const std::string& label) {
    MenuItem item;
    item.id = id;
    item.label = label;
    return item;
}

} // namespace

PauseMenu::PauseMenu(const UiStrings& strings)
    : main_(strings.paused, {action(kContinue, strings.continueGame), action(kAbort, strings.abortSkirmish)}),
      confirm_(strings.areYouSure, {action(kYes, strings.yes), action(kNo, strings.no)}) {
    reset();
}

void PauseMenu::reset() {
    confirming_ = false;
    main_.select(kContinue);
    confirm_.select(kNo);  // a quick double Enter must not end the match
}

PauseResult PauseMenu::handle(MenuInput input) {
    PauseResult result;
    if (!confirming_) {
        const auto response = main_.handle(input);
        if (response.type == Type::Moved) {
            result.sound = MenuSound::Move;
        } else if (response.type == Type::Back ||
                   (response.type == Type::Activated && response.itemId == kContinue)) {
            result.request = Request::Resume;
            result.sound = MenuSound::Back;
        } else if (response.type == Type::Activated && response.itemId == kAbort) {
            confirming_ = true;
            confirm_.select(kNo);
            result.sound = MenuSound::Select;
        }
        return result;
    }

    const auto response = confirm_.handle(input);
    if (response.type == Type::Moved) {
        result.sound = MenuSound::Move;
    } else if (response.type == Type::Activated && response.itemId == kYes) {
        result.request = Request::Abort;
        result.sound = MenuSound::Select;
    } else if (response.type == Type::Back || response.type == Type::Activated) {
        confirming_ = false;
        result.sound = MenuSound::Back;
    }
    return result;
}

} // namespace openhogs::ui

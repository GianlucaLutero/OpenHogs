// PauseMenu.cpp - see PauseMenu.h for an overview.

#include "openhogs/ui/PauseMenu.h"

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = PauseResult::Request;

MenuItem action(int id, const std::string& label) {
    MenuItem item;
    item.id = id;
    item.label = label;
    return item;
}

MenuItem slider(int id, const std::string& label) {
    MenuItem item = action(id, label);
    item.kind = MenuItem::Kind::Slider;
    item.maxValue = kMaxVolumeLevel;
    return item;
}

MenuItem speechItem(const UiStrings& strings) {
    MenuItem item = action(PauseMenu::kSpeech, strings.speech);
    item.kind = MenuItem::Kind::Choice;
    item.choices = {strings.speechOn, strings.speechOff};
    return item;
}

} // namespace

PauseMenu::PauseMenu(const UiStrings& strings, bool campaign)
    : main_(strings.paused, {action(kContinue, strings.continueGame), slider(kMasterVolume, strings.masterVolume),
                             slider(kSfxVolume, strings.sfxVolume), speechItem(strings),
                             action(kAbort, campaign ? strings.abortMission : strings.abortSkirmish)}),
      confirm_(strings.areYouSure, {action(kYes, strings.pauseYes), action(kNo, strings.pauseNo)}) {
    reset(VolumeLevels{});
}

void PauseMenu::reset(const VolumeLevels& volumes) {
    confirming_ = false;
    volumes_ = volumes;
    if (volumes.speech > 0) speechLevel_ = volumes.speech;
    main_.setValue(kMasterVolume, volumes.master);
    main_.setValue(kSfxVolume, volumes.effects);
    main_.setValue(kSpeech, volumes.speech > 0 ? 0 : 1);
    main_.select(kContinue);
    confirm_.select(kNo);  // a quick double Enter must not end the match
}

int PauseMenu::litNotches(Item volumeItem) const {
    return main_.item(volumeItem).value * kVolumeNotches / kMaxVolumeLevel;
}

PauseResult PauseMenu::handle(MenuInput input) {
    PauseResult result;
    if (!confirming_) {
        const auto response = main_.handle(input);
        if (response.type == Type::Moved) {
            result.sound = MenuSound::Move;
        } else if (response.type == Type::Changed) {
            volumes_.master = main_.item(kMasterVolume).value;
            volumes_.effects = main_.item(kSfxVolume).value;
            const bool speechOn = main_.item(kSpeech).value == 0;
            volumes_.speech = speechOn ? speechLevel_ : 0;
            result.request = Request::VolumesChanged;
            result.volumes = volumes_;
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

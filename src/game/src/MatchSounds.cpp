// MatchSounds.cpp - see MatchSounds.h for an overview, docs/formats/audio.md
// for where the rules come from.

#include "openhogs/game/MatchSounds.h"

#include <array>
#include <cstdio>

namespace openhogs::game {

namespace {

// Accents in scene::characterTeams() order: british, american, french,
// german, japanese, russian, team lard (the executable's table at VA
// 0x4C2988 also has "TL" for Team Lard, but no files were recorded).
constexpr std::array<std::string_view, 7> kAccents{"EN", "AM", "fr", "ge", "JA", "RU", ""};

// The executable's nation order (British, American, French, German,
// Russian, Japanese, Team Lard) for a scene::characterTeams() index.
constexpr std::array<int, 7> kExecutableNation{0, 1, 2, 3, 5, 4, 6};

int wrapNation(int nation) {
    const int count = static_cast<int>(kAccents.size());
    return ((nation % count) + count) % count;
}

// Health below which the "ready" line sounds tired, then desperate (the
// executable compares 3200 and 1280 in units of 1/64 point).
constexpr float kTiredHealth = 50.0f;
constexpr float kDesperateHealth = 20.0f;

// Sergeant sections (file SGEN<section + 1>).
constexpr int kScornSection = 0;
constexpr int kPraiseSection = 2;
constexpr int kHurrySection = 12;       // and 13: sixteen lines
constexpr int kFirstNationSection = 14; // + the executable's nation

// Seconds left in which a dawdler is told off, and the shortest turn this
// applies to (the executable: 1000 to 600 hundredths, turns over 1500).
constexpr double kHurryFrom = 10.0;
constexpr double kHurryUntil = 6.0;
constexpr double kHurryMinimumTurn = 15.0;

// Music tracks (MUSIC/TrackNN.ogg = CD track NN; the executable plays clip
// c as CD track c + 3).
constexpr int kFirstBattleTrack = 27;  // clips 24 .. 27
constexpr int kFirstNationTrack = 3;   // clip 4 x nation + tune
constexpr int kTunesPerNation = 4;

// Background, in frames of 1/25 s (FUN_00491b00).
constexpr int kNatureEvery = 251;
constexpr int kLoudBattleEvery = 101;
constexpr int kFaintBattleEvery = 126;

} // namespace

std::string_view speechAccent(int nation) { return kAccents[static_cast<std::size_t>(wrapNation(nation))]; }

int hogVoice(int indexInTeam) {
    return ((indexInTeam % kHogVoiceCount) + kHogVoiceCount) % kHogVoiceCount + 1;
}

std::string hogSpeechPath(int voice, int nation, int group, int variant) {
    const auto accent = speechAccent(nation);
    if (accent.empty() || voice < 1 || voice > kHogVoiceCount) return {};
    // As the executable: six variants per group, five groups.
    while (variant > 6) {
        ++group;
        variant -= 6;
    }
    int fileGroup = group + 1;
    if (fileGroup > 5) fileGroup = 1;
    char name[64];
    std::snprintf(name, sizeof name, "Speech/Sku1/Pig%02d/%02d%.2s%02d%02d.wav", voice, voice, accent.data(),
                  fileGroup, variant);
    return name;
}

std::string HogSpeech::next(HogLine kind, int voice, int nation, float health) {
    switch (kind) {
    case HogLine::Ready: {
        // Two lines for each state of health.
        int variant = readyCount_ + 1;
        if (health < kDesperateHealth) {
            variant += 4;
        } else if (health < kTiredHealth) {
            variant += 2;
        }
        readyCount_ = (readyCount_ + 1) % 2;
        return hogSpeechPath(voice, nation, 0, variant);
    }
    case HogLine::Attack: {
        const int variant = attackCount_ + 1;
        attackCount_ = (attackCount_ + 1) % 12;
        return hogSpeechPath(voice, nation, 1, variant);
    }
    case HogLine::Pain: {
        const int variant = painCount_ + 1;
        painCount_ = (painCount_ + 1) % 12;
        return hogSpeechPath(voice, nation, 3, variant);
    }
    }
    return {};
}

std::string sargeSpeechPath(int section, int variant) {
    while (variant > 8) {
        ++section;
        variant -= 8;
    }
    char name[64];
    std::snprintf(name, sizeof name, "Speech/Sku1/Sarge/SGEN%02d%02d.wav", section + 1, variant);
    return name;
}

int turnVerdict(const TurnOutcome& outcome) {
    const bool hitEnemy = outcome.enemiesKilled > 0 || outcome.enemiesHurt > 0;
    const bool hitFriends = outcome.friendsKilled > 0 || outcome.friendsHurt > 0;
    if (hitEnemy && !hitFriends) return 1;
    if (hitFriends && !hitEnemy) return -1;
    return 0;
}

std::optional<std::string> Sergeant::afterTurn(const TurnOutcome& outcome, bool singlePlayer, int nation) {
    const int verdict = turnVerdict(outcome);
    if (verdict == 0) return std::nullopt;

    if (singlePlayer) {
        // General praise and scorn, eight lines each, in turn.
        int& count = verdict > 0 ? praise_ : scorn_;
        const auto line = sargeSpeechPath(verdict > 0 ? kPraiseSection : kScornSection, count);
        count = count % 8 + 1;
        return line;
    }
    // Several people: lines 1-4 of the nation's section praise, 5-8 scorn.
    const int section = kFirstNationSection + kExecutableNation[static_cast<std::size_t>(wrapNation(nation))];
    if (verdict > 0) {
        const auto line = sargeSpeechPath(section, nationPraise_);
        nationPraise_ = nationPraise_ % 4 + 1;
        return line;
    }
    const auto line = sargeSpeechPath(section, nationScorn_);
    nationScorn_ = nationScorn_ >= 8 ? 5 : nationScorn_ + 1;
    return line;
}

std::optional<std::string> Sergeant::hurryUp(double secondsLeft, double turnSeconds, bool attacked,
                                             bool severalPeople) {
    if (!severalPeople || attacked || warnedThisTurn_ || turnSeconds <= kHurryMinimumTurn) return std::nullopt;
    if (secondsLeft > kHurryFrom || secondsLeft < kHurryUntil) return std::nullopt;
    warnedThisTurn_ = true;
    const auto line = sargeSpeechPath(kHurrySection, hurry_);
    hurry_ = hurry_ % 16 + 1;
    return line;
}

int MusicDirector::matchStarted(std::mt19937& random) const {
    return kFirstBattleTrack + static_cast<int>(random() % 4);
}

int MusicDirector::computerTurnStarted(int team, int nation) {
    if (team < 0) team = 0;
    if (static_cast<std::size_t>(team) >= nextTune_.size()) {
        nextTune_.resize(static_cast<std::size_t>(team) + 1, 0);
    }
    int& tune = nextTune_[static_cast<std::size_t>(team)];
    const int exeNation = kExecutableNation[static_cast<std::size_t>(wrapNation(nation))];
    const int track = kFirstNationTrack + exeNation * kTunesPerNation + tune;
    tune = (tune + 1) % kTunesPerNation;
    return track;
}

int MusicDirector::matchOver(bool personWon) const { return personWon ? kVictoryTrack : kThemeTrack; }

std::vector<AmbientSound> AmbienceScheduler::advance(double seconds, std::mt19937& random) {
    std::vector<AmbientSound> sounds;
    carried_ += seconds > 0.0 ? seconds : 0.0;
    while (carried_ >= kTickSeconds) {
        carried_ -= kTickSeconds;
        if (auto sound = tick(random)) sounds.push_back(*sound);
    }
    return sounds;
}

std::optional<AmbientSound> AmbienceScheduler::tick(std::mt19937& random) {
    // The volume (25 .. 40 of 100) and pitch (90 .. 121 %, nature 90 ..
    // 105 %) vary as in the executable.
    const auto make = [&random](std::string name, int pitchRange) {
        AmbientSound sound;
        sound.name = std::move(name);
        sound.volume = static_cast<float>(25 + random() % 16) / 100.0f;
        sound.pitch = static_cast<float>(90 + random() % static_cast<unsigned>(pitchRange)) / 100.0f;
        return sound;
    };
    std::optional<AmbientSound> sound;
    // The executable looks at one counter per frame, in this order.
    if (natureTicks_ >= kNatureEvery - 1) {
        if (random() % 16 == 0) sound = make(random() % 2 == 0 ? "AMB_1D" : "AMB_2D", 16);
        natureTicks_ = -1;
    } else if (loudTicks_ >= kLoudBattleEvery - 1) {
        if (random() % 2 == 0) sound = make("BATT_L" + std::to_string(1 + random() % 3), 32);
        loudTicks_ = -1;
    } else if (faintTicks_ >= kFaintBattleEvery - 1) {
        if (random() % 2 == 0) sound = make("BATT_S" + std::to_string(1 + random() % 3), 32);
        faintTicks_ = -1;
    }
    ++natureTicks_;
    ++loudTicks_;
    ++faintTicks_;
    return sound;
}

} // namespace openhogs::game

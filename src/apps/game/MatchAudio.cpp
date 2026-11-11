// MatchAudio.cpp - see MatchAudio.h for an overview.

#include "MatchAudio.h"

#include <map>
#include <set>

namespace openhogs::gameapp {

namespace {

// Volumes of the executable, out of 100.
constexpr float kBattleTuneVolume = 0.7f;   // 0x46
constexpr float kVictoryTuneVolume = 0.8f;  // 0x50
constexpr float kThinkingVolume = 0.6f;     // P_HMMM, 0x3C

constexpr std::string_view kThinkingSound = "P_HMMM";
constexpr std::string_view kPainSound = "P_OWW";  // when the hog cannot say its line

} // namespace

MatchAudio::MatchAudio(SoundPlayer& sounds, const game::World& world, std::function<bool(int)> isComputerTeam)
    : sounds_(sounds), world_(world), isComputerTeam_(std::move(isComputerTeam)), random_(std::random_device{}()) {
    // Each hog gets a voice: campaign hogs by their place in the squad,
    // the others by their place in their team.
    std::map<int, int> hogsSeen;  // per team
    std::set<int> teams;
    for (const auto& hog : world_.hogs()) {
        const int indexInTeam = hogsSeen[hog.team]++;
        voices_.push_back(game::hogVoice(hog.squadIndex ? static_cast<int>(*hog.squadIndex) : indexInTeam));
        teams.insert(hog.team);
    }
    speech_.resize(world_.hogs().size());
    for (const int team : teams) {
        if (isPerson(team)) ++people_;
    }
}

void MatchAudio::matchStarted() {
    sounds_.playMusic(music_.matchStarted(random_), kBattleTuneVolume);
    turnStarted();
}

bool MatchAudio::say(std::size_t hog, game::HogLine line) {
    if (hog >= world_.hogs().size() || sounds_.speaking()) return false;
    const auto& who = world_.hogs()[hog];
    const auto path = speech_[hog].next(line, voices_[hog], world_.nationOf(who.team), who.health);
    return sounds_.say(path);
}

void MatchAudio::turnStarted() {
    outcome_ = {};
    attacked_ = false;
    sergeant_.turnStarted();
    const auto& turns = world_.turns();
    if (!turns || world_.phase() == game::MatchPhase::Over) {
        turnTeam_ = -1;
        return;
    }
    turnTeam_ = turns->activeTeam();
    if (isPerson(turnTeam_)) {
        say(turns->activeHog(), game::HogLine::Ready);
    } else {
        // The computer's hog thinks aloud and its nation's tune plays.
        const float pitch = static_cast<float>(90 + random_() % 16) / 100.0f;
        sounds_.play(SoundBankId::Match, kThinkingSound, kThinkingVolume, pitch);
        sounds_.playMusic(music_.computerTurnStarted(turnTeam_, world_.nationOf(turnTeam_)), kBattleTuneVolume);
    }
}

void MatchAudio::turnEnded() {
    if (turnTeam_ < 0) return;
    // In a match against the computer the sergeant only speaks to the
    // person; between people he comments on everyone.
    const bool singlePlayer = people_ <= 1;
    if (singlePlayer && !isPerson(turnTeam_)) return;
    if (const auto line = sergeant_.afterTurn(outcome_, singlePlayer, world_.nationOf(turnTeam_))) {
        sounds_.say(*line);
    }
}

void MatchAudio::handle(const game::GameEvent& event) {
    using game::GameEventType;
    switch (event.type) {
    case GameEventType::TurnStarted:
        turnEnded();
        turnStarted();
        break;
    case GameEventType::Fired:
        if (!attacked_) {
            attacked_ = true;
            if (const auto active = world_.activeHog()) say(*active, game::HogLine::Attack);
        }
        break;
    case GameEventType::Hurt:
    case GameEventType::Died:
        if (event.hog < world_.hogs().size()) {
            const bool friendly = world_.hogs()[event.hog].team == turnTeam_;
            const bool died = event.type == GameEventType::Died;
            int& count = friendly ? (died ? outcome_.friendsKilled : outcome_.friendsHurt)
                                  : (died ? outcome_.enemiesKilled : outcome_.enemiesHurt);
            ++count;
            // A hog without recorded lines (Team Lard), or hurt while
            // someone else speaks, only cries out.
            if (!died && !say(event.hog, game::HogLine::Pain)) {
                sounds_.play(SoundBankId::Match, kPainSound);
            }
        }
        break;
    case GameEventType::MatchOver: {
        const bool personWon = event.team >= 0 && isPerson(event.team);
        sounds_.playMusic(music_.matchOver(personWon), personWon ? kVictoryTuneVolume : 1.0f);
        turnTeam_ = -1;
        break;
    }
    default:
        break;
    }
}

void MatchAudio::update(double seconds) {
    for (const auto& sound : ambience_.advance(seconds, random_)) {
        sounds_.play(SoundBankId::Match, sound.name, sound.volume, sound.pitch);
    }
    const auto& turns = world_.turns();
    if (turns && turnTeam_ >= 0 && world_.phase() == game::MatchPhase::Playing) {
        if (const auto line = sergeant_.hurryUp(turns->secondsLeft(), turns->rules().turnSeconds, attacked_,
                                                people_ > 1)) {
            sounds_.say(*line);
        }
    }
}

} // namespace openhogs::gameapp

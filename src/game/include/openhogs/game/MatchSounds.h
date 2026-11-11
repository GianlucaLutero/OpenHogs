// MatchSounds.h
//
// Which voice, music and background sound the original game plays during
// a match, and when, as read from warhogs_.exe (docs/formats/audio.md,
// section "When the game plays what"). Only the choices are made here: the
// game application plays the files.
//
//   - Hog voices (Speech/Sku1/PigNN): each hog speaks with one of nine
//     voices in its nation's accent. It says a "ready" line when its turn
//     starts (cheerful, tired or desperate depending on its health), a
//     battle cry when it attacks, and a cry of pain when it is hurt. Each
//     hog goes through its lines in turn, so it does not repeat itself.
//   - The sergeant (Speech/Sku1/Sarge) comments on a turn of a team played
//     by a person once its shot is over: praise when it hurt the enemy,
//     scorn when it hurt its own side. In a match between several people
//     each nation has its own lines, and he tells off a player who dawdles.
//   - Music (MUSIC/TrackNN.ogg, the tracks of the original CD): a battle
//     tune when the match starts, a nation's tune when the computer's turn
//     starts, the victory tune when a person wins, the long theme
//     otherwise. The menus play the theme in a loop.
//   - Background: no continuous ambience; from time to time distant battle
//     noise or a sound of nature, quiet and at a slightly random pitch.

#pragma once

#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::game {

// --- Hog voices ----------------------------------------------------------

// The nine voices; 0 = none (Team Lard has no recorded lines).
inline constexpr int kHogVoiceCount = 9;

// The accent of a nation (index into scene::characterTeams()) in speech
// file names: "EN", "AM", "fr", "ge", "JA", "RU"; empty for Team Lard.
std::string_view speechAccent(int nation);

// The voice of the `indexInTeam`-th hog of a team (1 .. 9).
int hogVoice(int indexInTeam);

// The file of a hog's line: voice 1 .. 9, `group` and `variant` as the
// executable counts them (group 0 .., variant 1 ..; a variant past 6 goes
// on in the next group, and past group 5 the groups start again), e.g.
// "Speech/Sku1/Pig03/03EN0201.wav". Empty when the nation has no accent.
std::string hogSpeechPath(int voice, int nation, int group, int variant);

// What a hog says.
enum class HogLine {
    Ready,   // its turn starts (group 0; the variant depends on health)
    Attack,  // it uses a weapon (groups 1-2, twelve lines)
    Pain,    // it was hurt (groups 3-4, twelve lines)
};

// The lines one hog has said, so the next one is another.
class HogSpeech {
public:
    // The file of the next line of `kind` for a hog with `voice` and
    // `nation` and `health` points left. Empty when there is none.
    std::string next(HogLine kind, int voice, int nation, float health);

private:
    int readyCount_ = 0;   // 0 .. 1
    int attackCount_ = 0;  // 0 .. 11
    int painCount_ = 0;    // 0 .. 11
};

// --- The sergeant ----------------------------------------------------------

// The file of a sergeant line: `section` 0 .. 21, `variant` 1 .. 8 (past 8
// it goes on in the next section), e.g. "Speech/Sku1/Sarge/SGEN0301.wav".
std::string sargeSpeechPath(int section, int variant);

// What a team's turn did to the hogs.
struct TurnOutcome {
    int enemiesKilled = 0;
    int enemiesHurt = 0;
    int friendsKilled = 0;
    int friendsHurt = 0;
};

// +1: it hurt the enemy and not its own side, -1: it hurt its own side
// and not the enemy, 0: neither or both.
int turnVerdict(const TurnOutcome& outcome);

class Sergeant {
public:
    // The line once a person's turn is over, or nothing. `singlePlayer`:
    // one person against the computer (the campaign or a skirmish), whose
    // lines are general; otherwise the lines of `nation` (index into
    // scene::characterTeams()).
    std::optional<std::string> afterTurn(const TurnOutcome& outcome, bool singlePlayer, int nation);

    // In a match between several people, a dawdler is told off once per
    // turn when 10 to 6 seconds are left and the hog has not attacked
    // (only for turns of more than 15 seconds).
    std::optional<std::string> hurryUp(double secondsLeft, double turnSeconds, bool attacked, bool severalPeople);

    // A new turn: the dawdling warning may be given again.
    void turnStarted() { warnedThisTurn_ = false; }

private:
    int praise_ = 1;          // single player: section 2
    int scorn_ = 1;           // single player: section 0
    int nationPraise_ = 1;    // several players: 1 .. 4 of the nation's section
    int nationScorn_ = 5;     // several players: 5 .. 8
    int hurry_ = 1;           // sections 12-13, sixteen lines
    bool warnedThisTurn_ = false;
};

// --- Music -----------------------------------------------------------------

// The track numbers of MUSIC/TrackNN.ogg.
inline constexpr int kThemeTrack = 2;    // menus, and the end of a lost match
inline constexpr int kVictoryTrack = 31;

class MusicDirector {
public:
    // A battle tune for the start of a match (tracks 27 .. 30).
    int matchStarted(std::mt19937& random) const;

    // The tune of `nation` when the computer's turn of `team` starts: each
    // nation has four, which the team plays in turn.
    int computerTurnStarted(int team, int nation);

    // The end of the match: the victory tune if a person won.
    int matchOver(bool personWon) const;

private:
    std::vector<int> nextTune_;  // per team, 0 .. 3
};

// --- Background ------------------------------------------------------------

struct AmbientSound {
    std::string name;     // in Audio/sfxday.srl, e.g. "BATT_L2"
    float volume = 0.0f;  // 0 .. 1
    float pitch = 1.0f;
};

// Distant battle noise and sounds of nature. The executable counts frames
// (25 per second): every 251 one chance in 16 of AMB_1D/2D, otherwise
// every 101 one chance in 2 of a loud distant battle (BATT_L1-3), every
// 126 one chance in 2 of a faint one (BATT_S1-3); 25 to 40 % loud.
class AmbienceScheduler {
public:
    static constexpr double kTickSeconds = 1.0 / 25.0;

    // The sounds to start after `seconds` more of the match.
    std::vector<AmbientSound> advance(double seconds, std::mt19937& random);

private:
    std::optional<AmbientSound> tick(std::mt19937& random);

    double carried_ = 0.0;
    int natureTicks_ = 0;
    int loudTicks_ = 0;
    int faintTicks_ = 0;
};

} // namespace openhogs::game

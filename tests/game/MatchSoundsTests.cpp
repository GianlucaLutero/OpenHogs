// Tests for game::MatchSounds: which voice, music and background sound
// plays when, as in the original executable.
//
// A wrong file name means silence (or the wrong nation's accent), and a
// wrong rule means hogs repeating themselves or the sergeant praising a
// player who just blew up his own side. These rules were read from the
// executable and are easy to break when touching the counters.

#include "openhogs/game/MatchSounds.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace openhogs::game;

TEST_CASE("Hog speech files follow the game's naming", "[game][matchsounds]") {
    // Speech/Sku1/PigNN/NN<accent><group><variant>.wav, with the accents
    // of the nations (the letter case of the files varies; the file system
    // ignores it) and none for Team Lard, which has no recorded lines.
    CHECK(hogSpeechPath(3, 0, 0, 1) == "Speech/Sku1/Pig03/03EN0101.wav");
    CHECK(hogSpeechPath(1, 2, 1, 2) == "Speech/Sku1/Pig01/01fr0202.wav");
    CHECK(hogSpeechPath(9, 4, 3, 6) == "Speech/Sku1/Pig09/09JA0406.wav");  // Japanese
    CHECK(hogSpeechPath(9, 5, 3, 6) == "Speech/Sku1/Pig09/09RU0406.wav");  // Russian
    CHECK(hogSpeechPath(1, 6, 0, 1).empty());
    // A variant past 6 goes on in the next group, as in the executable.
    CHECK(hogSpeechPath(2, 1, 1, 7) == "Speech/Sku1/Pig02/02AM0301.wav");
    CHECK(hogSpeechPath(2, 1, 3, 12) == "Speech/Sku1/Pig02/02AM0506.wav");
}

TEST_CASE("Hogs get one of the nine voices", "[game][matchsounds]") {
    // There are nine voice folders: hog ten must not ask for a tenth.
    CHECK(hogVoice(0) == 1);
    CHECK(hogVoice(8) == 9);
    CHECK(hogVoice(9) == 1);
}

TEST_CASE("The ready line depends on the hog's health", "[game][matchsounds]") {
    // A healthy hog sounds cheerful (lines 1-2), a wounded one tired
    // (3-4), a dying one desperate (5-6); it alternates its two lines.
    HogSpeech speech;
    CHECK(speech.next(HogLine::Ready, 1, 0, 100.0f) == "Speech/Sku1/Pig01/01EN0101.wav");
    CHECK(speech.next(HogLine::Ready, 1, 0, 100.0f) == "Speech/Sku1/Pig01/01EN0102.wav");
    CHECK(speech.next(HogLine::Ready, 1, 0, 30.0f) == "Speech/Sku1/Pig01/01EN0103.wav");
    CHECK(speech.next(HogLine::Ready, 1, 0, 10.0f) == "Speech/Sku1/Pig01/01EN0106.wav");
}

TEST_CASE("Battle cries and cries of pain go through twelve lines", "[game][matchsounds]") {
    // Each hog says all its lines before repeating one.
    HogSpeech speech;
    std::set<std::string> attacks;
    for (int i = 0; i < 12; ++i) attacks.insert(speech.next(HogLine::Attack, 4, 3, 100.0f));
    CHECK(attacks.size() == 12);
    CHECK(attacks.count("Speech/Sku1/Pig04/04ge0201.wav") == 1);
    CHECK(attacks.count("Speech/Sku1/Pig04/04ge0306.wav") == 1);
    CHECK(speech.next(HogLine::Attack, 4, 3, 100.0f) == "Speech/Sku1/Pig04/04ge0201.wav");
    CHECK(speech.next(HogLine::Pain, 4, 3, 100.0f) == "Speech/Sku1/Pig04/04ge0401.wav");
}

TEST_CASE("The sergeant praises, scorns or says nothing", "[game][matchsounds]") {
    // Against the computer he uses his general lines; between people the
    // lines of the team's nation, 1-4 to praise and 5-8 to scorn. A turn
    // that hurt nobody, or both sides, gets no comment.
    Sergeant sergeant;
    TurnOutcome good;
    good.enemiesHurt = 1;
    TurnOutcome bad;
    bad.friendsKilled = 1;
    TurnOutcome mixed;
    mixed.enemiesHurt = 1;
    mixed.friendsHurt = 1;

    CHECK(sergeant.afterTurn(good, true, 0) == "Speech/Sku1/Sarge/SGEN0301.wav");
    CHECK(sergeant.afterTurn(good, true, 0) == "Speech/Sku1/Sarge/SGEN0302.wav");
    CHECK(sergeant.afterTurn(bad, true, 0) == "Speech/Sku1/Sarge/SGEN0101.wav");
    CHECK_FALSE(sergeant.afterTurn(mixed, true, 0));
    CHECK_FALSE(sergeant.afterTurn(TurnOutcome{}, true, 0));

    // Russian (characterTeams index 5) is the executable's nation 4.
    CHECK(sergeant.afterTurn(good, false, 5) == "Speech/Sku1/Sarge/SGEN1901.wav");
    CHECK(sergeant.afterTurn(bad, false, 0) == "Speech/Sku1/Sarge/SGEN1505.wav");
}

TEST_CASE("A dawdler is told off once per turn", "[game][matchsounds]") {
    // Only between people, only between 10 and 6 seconds left, only if
    // the hog has not attacked, and not on short turns.
    Sergeant sergeant;
    CHECK_FALSE(sergeant.hurryUp(8.0, 60.0, false, false));
    CHECK_FALSE(sergeant.hurryUp(12.0, 60.0, false, true));
    CHECK_FALSE(sergeant.hurryUp(8.0, 60.0, true, true));
    CHECK_FALSE(sergeant.hurryUp(8.0, 15.0, false, true));
    CHECK(sergeant.hurryUp(8.0, 60.0, false, true) == "Speech/Sku1/Sarge/SGEN1301.wav");
    CHECK_FALSE(sergeant.hurryUp(7.0, 60.0, false, true));
    sergeant.turnStarted();
    CHECK(sergeant.hurryUp(8.0, 60.0, false, true) == "Speech/Sku1/Sarge/SGEN1302.wav");
}

TEST_CASE("Music follows the executable's clips", "[game][matchsounds]") {
    // A battle tune (27-30) at the start; each team of the computer plays
    // its nation's four tunes in turn; victory or the theme at the end.
    std::mt19937 random(7);
    MusicDirector music;
    for (int i = 0; i < 20; ++i) {
        const int track = music.matchStarted(random);
        CHECK(track >= 27);
        CHECK(track <= 30);
    }
    CHECK(music.computerTurnStarted(1, 0) == 3);   // British: 3-6
    CHECK(music.computerTurnStarted(1, 0) == 4);
    CHECK(music.computerTurnStarted(2, 4) == 23);  // Japanese: the executable's nation 5
    CHECK(music.computerTurnStarted(1, 0) == 5);
    CHECK(music.computerTurnStarted(1, 0) == 6);
    CHECK(music.computerTurnStarted(1, 0) == 3);
    CHECK(music.matchOver(true) == kVictoryTrack);
    CHECK(music.matchOver(false) == kThemeTrack);
}

TEST_CASE("Background sounds come now and then, quiet", "[game][matchsounds]") {
    // About one distant battle every 4-5 seconds, never in the first four
    // seconds, always quiet; no continuous loop.
    std::mt19937 random(3);
    AmbienceScheduler ambience;
    CHECK(ambience.advance(3.9, random).empty());
    const auto sounds = ambience.advance(120.0, random);
    CHECK(sounds.size() > 10);
    CHECK(sounds.size() < 40);
    for (const auto& sound : sounds) {
        CHECK(sound.volume >= 0.25f);
        CHECK(sound.volume <= 0.40f);
        CHECK(sound.pitch >= 0.9f);
        CHECK(sound.pitch <= 1.21f);
        CHECK((sound.name.rfind("BATT_", 0) == 0 || sound.name.rfind("AMB_", 0) == 0));
    }
}

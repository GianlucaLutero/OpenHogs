# openhogs-soundtest: plays (or renders to a WAV file) the game's sound
# effects, speech and music. A "sound test" like old console games had,
# and a quick way to check content packs that replace sounds.

add_executable(openhogs-soundtest main.cpp)
target_link_libraries(openhogs-soundtest PRIVATE
    OpenHogs::content
    OpenHogs::audio
    OpenHogs::audio_device)
openhogs_set_warnings(openhogs-soundtest)

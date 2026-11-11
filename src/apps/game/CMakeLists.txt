# openhogs: the game itself. For now it loads a map, puts the hogs on it
# and lets the player look around; the next blocks add movement, weapons
# and the rest.
#
# The match state lives in the game module, drawing in the render module,
# sound in the audio module; this application connects them to the window
# and the input.

add_executable(openhogs
    main.cpp
    Game.cpp)

target_link_libraries(openhogs PRIVATE
    OpenHogs::app_common
    OpenHogs::game
    OpenHogs::render
    OpenHogs::audio
    OpenHogs::audio_device
    SDL3::SDL3)
openhogs_set_warnings(openhogs)

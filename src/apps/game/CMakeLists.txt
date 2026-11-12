# openhogs: the game itself. Main menu, level choice and volume options,
# then hot-seat matches with a HUD and a pause menu.
#
#   Application    - window, sound, interface; switches menus <-> match
#   CampaignSession - a campaign: squad pages, briefings, missions, saving
#   CutscenePlayer - the original's videos (Bink), full screen with sound
#   Game           - one match: world, camera, drawing, HUD, pause menu
#   Gamepads       - game controllers for the menus and the matches
#   Interface      - fonts, icons, texts and shared drawing of the 2D interface
#   MatchAudio     - voices, music and background sounds of a match
#   MenuBackground - the map circling behind the menus
#   OriginalHud    - the original's HUD instruments: angle and power gauges, clock
#   SoundPlayer    - mixer, sound device, sounds by name from the banks
#   MenuControls   - menu keys and sounds
#
# The match state lives in the game module, menus and HUD logic in the ui
# module, drawing in the render module, sound in the audio module; this
# application connects them to the window and the input.

add_executable(openhogs
    main.cpp
    Application.cpp
    CampaignSession.cpp
    CutscenePlayer.cpp
    Game.cpp
    Gamepads.cpp
    Interface.cpp
    MatchAudio.cpp
    MenuBackground.cpp
    OriginalHud.cpp
    SoundPlayer.cpp)

target_link_libraries(openhogs PRIVATE
    OpenHogs::app_common
    OpenHogs::game
    OpenHogs::render
    OpenHogs::ui
    OpenHogs::video
    OpenHogs::audio
    OpenHogs::audio_device
    SDL3::SDL3)
openhogs_set_warnings(openhogs)

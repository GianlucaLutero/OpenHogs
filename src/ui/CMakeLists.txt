# ui: what the player sees and does around a match, with no drawing code,
# so it can be unit tested. The render module draws it; the game
# application feeds it the input.
#
#   TextLayout    - text to positioned font characters; ">S" placeholders
#   UiStrings     - menu and HUD texts from the game's text tables
#   Menu          - one menu screen: items, selection, sliders
#   FrontEnd      - the menus before a match (main menu, levels, options)
#   PauseMenu     - the menu over a paused match
#   MessageBanner - timed messages in the middle of the screen
#   Hud           - what the HUD shows; messages for game events
#   CampaignMenu  - the campaign pages: squad, promotions, mission results

add_library(openhogs_ui STATIC
    src/CampaignMenu.cpp
    src/FrontEnd.cpp
    src/Hud.cpp
    src/Menu.cpp
    src/MessageBanner.cpp
    src/PauseMenu.cpp
    src/TextLayout.cpp
    src/UiStrings.cpp)
add_library(OpenHogs::ui ALIAS openhogs_ui)

target_include_directories(openhogs_ui PUBLIC include)
target_link_libraries(openhogs_ui PUBLIC OpenHogs::formats OpenHogs::game)
openhogs_set_warnings(openhogs_ui)

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
#   FloatingNumbers - damage numbers rising above hurt hogs
#   Hud           - what the HUD shows; messages for game events
#   HudLayout     - the original HUD's layout and moving parts (from the executable)
#   OriginalMenu  - the original list menu's layout and animation (from the executable)
#   OriginalDialog - the original yes/no question's layout and animation
#   OriginalVolume - the original VOLUME CONTROL panel's layout and animation
#   OriginalSelectList - the original longer list (SELECT TEAM), layout and animation
#   OriginalLevelPanel - the original level panel (SELECT LEVEL, FIELD CONDITIONS) and team holders
#   OriginalOptionPages - the original CREDITS roll and CONTROLS board
#   OriginalSquad - the original campaign squad page and a hog's page
#   CampaignMenu  - the campaign pages: squad, promotions, mission results

add_library(openhogs_ui STATIC
    src/CampaignMenu.cpp
    src/FloatingNumbers.cpp
    src/FrontEnd.cpp
    src/Hud.cpp
    src/HudLayout.cpp
    src/Menu.cpp
    src/OriginalDialog.cpp
    src/OriginalVolume.cpp
    src/OriginalSelectList.cpp
    src/OriginalLevelPanel.cpp
    src/OriginalOptionPages.cpp
    src/OriginalSquad.cpp
    src/OriginalMenu.cpp
    src/MessageBanner.cpp
    src/PauseMenu.cpp
    src/TextLayout.cpp
    src/UiStrings.cpp)
add_library(OpenHogs::ui ALIAS openhogs_ui)

target_include_directories(openhogs_ui PUBLIC include)
target_link_libraries(openhogs_ui PUBLIC OpenHogs::formats OpenHogs::game)
openhogs_set_warnings(openhogs_ui)

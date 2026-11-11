# game: the rules and state of a match, with no drawing, sound or window
# code, so everything here can be unit tested.
#
#   FixedTimestep      - runs the simulation at a fixed rate whatever the
#                        display's frame rate
#   TerrainHeightField - height and surface of the ground at any point
#   Hog                - one hog of a match; creation from the map's objects
#   IdleAnimator       - what a hog does while standing around
#   World              - the terrain and the hogs of the match being played
#   ChaseCamera        - the camera that follows the selected hog

add_library(openhogs_game STATIC
    src/ChaseCamera.cpp
    src/FixedTimestep.cpp
    src/Hog.cpp
    src/IdleAnimator.cpp
    src/TerrainHeightField.cpp
    src/World.cpp)
add_library(OpenHogs::game ALIAS openhogs_game)

target_include_directories(openhogs_game PUBLIC include)
target_link_libraries(openhogs_game PUBLIC OpenHogs::core OpenHogs::formats OpenHogs::scene)
openhogs_set_warnings(openhogs_game)

# game: the rules and state of a match, with no drawing, sound or window
# code, so everything here can be unit tested.
#
#   FixedTimestep      - runs the simulation at a fixed rate whatever the
#                        display's frame rate
#   TerrainHeightField - height and surface of the ground at any point
#   MapCollision       - terrain plus the objects' collision boxes
#   Hog                - one hog of a match; creation from the map's objects
#   AnimationPlayer    - plays and cross-fades a hog's animations
#   IdleAnimator       - what a hog does while standing around
#   HogMovement        - walking, turning, jumping, falling, swimming
#   TurnSystem         - whose turn it is, time and movement left
#   Weapons            - what each weapon does, who carries which
#   Combat             - damage, death, explosions, bullets
#   Projectiles        - grenades, shells, TNT and mines in flight or lying
#   World              - the map, the hogs and the turns of the match
#   ChaseCamera        - the camera that follows the active hog
#   ComputerPlayer     - the computer opponent: plans and plays a turn

add_library(openhogs_game STATIC
    src/AnimationPlayer.cpp
    src/ChaseCamera.cpp
    src/ComputerPlayer.cpp
    src/Combat.cpp
    src/FixedTimestep.cpp
    src/Hog.cpp
    src/HogMovement.cpp
    src/IdleAnimator.cpp
    src/MapCollision.cpp
    src/Projectiles.cpp
    src/TerrainHeightField.cpp
    src/TurnSystem.cpp
    src/Weapons.cpp
    src/World.cpp)
add_library(OpenHogs::game ALIAS openhogs_game)

target_include_directories(openhogs_game PUBLIC include)
target_link_libraries(openhogs_game PUBLIC OpenHogs::core OpenHogs::formats OpenHogs::scene)
openhogs_set_warnings(openhogs_game)

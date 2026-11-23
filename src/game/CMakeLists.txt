# game: the rules and state of a match, with no drawing, sound or window
# code, so everything here can be unit tested.
#
#   FixedTimestep      - runs the simulation at a fixed rate whatever the
#                        display's frame rate
#   TerrainHeightField - height and surface of the ground at any point
#   MapCollision       - terrain plus the objects' collision boxes
#   Pickups            - the crates of a map and what they hold
#   Hog                - one hog of a match; creation from the map's objects
#   AnimationPlayer    - plays and cross-fades a hog's animations
#   IdleAnimator       - what a hog does while standing around
#   HogMovement        - walking, turning, jumping, falling, swimming
#   TurnSystem         - whose turn it is and the time left
#   Weapons            - what each weapon does, who carries which
#   Combat             - damage, death, explosions, bullets
#   Projectiles        - grenades, shells, TNT and mines in flight or lying
#   Mounts             - the buildings and vehicles a hog can get into
#   GameVersion        - where the PC and PlayStation rules differ
#   World              - the map, the hogs and the turns of the match
#   ChaseCamera        - the camera that follows the active hog
#   ComputerPlayer     - the computer opponent: plans and plays a turn
#   Ranks              - campaign ranks, careers and promotions
#   Levels             - the game's numbered levels: maps, names, skies
#   Campaign           - the missions, the squad, points and saving
#   Cutscenes          - which videos the original plays, and when
#   Debrief            - the campaign's debriefing page: faces, badges
#   Newspaper          - which front page follows a campaign victory
#   FieldConditions    - multiplayer settings: teams off, hogs per team, health
#   MatchSounds        - which voice, music and background sound the original
#                        plays during a match (the files, not the playing)

add_library(openhogs_game STATIC
    src/AnimationPlayer.cpp
    src/BattleAwards.cpp
    src/Campaign.cpp
    src/ChaseCamera.cpp
    src/ComputerPlayer.cpp
    src/Cutscenes.cpp
    src/Debrief.cpp
    src/Destructibles.cpp
    src/Combat.cpp
    src/FieldConditions.cpp
    src/FixedTimestep.cpp
    src/Hog.cpp
    src/HogMovement.cpp
    src/IdleAnimator.cpp
    src/Levels.cpp
    src/MatchSounds.cpp
    src/MapCollision.cpp
    src/Newspaper.cpp
    src/Projectiles.cpp
    src/Ranks.cpp
    src/TerrainHeightField.cpp
    src/Training.cpp
    src/TurnSystem.cpp
    src/Objectives.cpp
    src/Mounts.cpp
    src/GameVersion.cpp
    src/Pickups.cpp
    src/Weapons.cpp
    src/World.cpp)
add_library(OpenHogs::game ALIAS openhogs_game)

target_include_directories(openhogs_game PUBLIC include)
target_link_libraries(openhogs_game PUBLIC OpenHogs::core OpenHogs::formats OpenHogs::scene)
openhogs_set_warnings(openhogs_game)

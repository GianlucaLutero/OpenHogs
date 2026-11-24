# scene: turns the original map data into geometry ready to draw.
#
# This module does all the "game data -> triangles" work on the CPU and has
# no dependency on OpenGL or SDL, so it can be unit tested and reused by
# any future renderer.
#
#   Geometry       - vertex and mesh types shared by the builders
#   TerrainMesh    - triangles of the terrain, grouped by texture
#   ModelMesh      - triangles of a 3D model, grouped by texture (also
#                    posed by a skeleton, for characters)
#   CharacterLibrary - loads the hogs' skeleton, animations, models and
#                    team textures from the virtual file system
#   CharacterExpressions - facial expressions (eye and mouth textures),
#                    blinking, rules per animation
#   CharacterPose  - bone transforms of a hog from its skeleton and an
#                    animation frame
#   Particles      - the effects' particles (fire, smoke, earth, foam, scorch marks)
#   MapScene       - loads a whole map (terrain, textures, objects) from the
#                    virtual file system
#   MapColours     - the level's recolouring of its textures (sunset, night)
#   SkyDome        - the dome of sky drawn around the map

add_library(openhogs_scene STATIC
    src/CharacterExpressions.cpp
    src/CharacterLibrary.cpp
    src/CharacterPose.cpp
    src/MapColours.cpp
    src/MapScene.cpp
    src/SkyDome.cpp
    src/Particles.cpp
    src/ModelMesh.cpp
    src/TerrainMesh.cpp)

add_library(OpenHogs::scene ALIAS openhogs_scene)

target_include_directories(openhogs_scene PUBLIC include)
target_link_libraries(openhogs_scene PUBLIC OpenHogs::core OpenHogs::formats glm::glm)
openhogs_set_warnings(openhogs_scene)

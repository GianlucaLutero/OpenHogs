# render: draws a scene::MapScene with OpenGL 3.3.
#
#   Camera       - free-flying camera (position + yaw/pitch)
#   GlObjects    - small RAII wrappers for OpenGL textures, buffers, shaders
#   MapRenderer  - uploads a MapScene to the GPU and draws it
#
# The OpenGL context itself is created by the application (with SDL), so
# this module does not depend on any windowing library.

add_library(openhogs_render STATIC
    src/Camera.cpp
    src/GlObjects.cpp
    src/MapRenderer.cpp)

add_library(OpenHogs::render ALIAS openhogs_render)

target_include_directories(openhogs_render PUBLIC include)
target_link_libraries(openhogs_render PUBLIC OpenHogs::scene glad)
openhogs_set_warnings(openhogs_render)

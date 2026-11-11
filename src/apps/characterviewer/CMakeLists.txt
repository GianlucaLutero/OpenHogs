# openhogs-characterviewer: shows one hog, animated, with keys to change
# its class, team, level of detail and animation.
#
# Posing is done by the scene module, drawing by the render module; SDL3
# handles input and the shared GlWindow creates the window.

add_executable(openhogs-characterviewer
    main.cpp
    CharacterViewer.cpp)

target_link_libraries(openhogs-characterviewer PRIVATE
    OpenHogs::app_common
    OpenHogs::render
    SDL3::SDL3)
openhogs_set_warnings(openhogs-characterviewer)

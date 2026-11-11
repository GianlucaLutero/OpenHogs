# openhogs-mapviewer: opens a window and shows an original map in 3D.
#
# SDL3 handles input, the shared GlWindow creates the window and the
# OpenGL context; drawing is done by the render module.

add_executable(openhogs-mapviewer
    main.cpp
    MapViewer.cpp)

target_link_libraries(openhogs-mapviewer PRIVATE
    OpenHogs::content
    OpenHogs::render
    OpenHogs::app_common
    SDL3::SDL3)
openhogs_set_warnings(openhogs-mapviewer)

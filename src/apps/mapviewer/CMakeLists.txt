# openhogs-mapviewer: opens a window and shows an original map in 3D.
#
# SDL3 creates the window, the OpenGL context and handles input; drawing
# is done by the render module. The PNG encoder of the extractor library
# is reused for screenshots.

add_executable(openhogs-mapviewer
    main.cpp
    MapViewer.cpp)

target_link_libraries(openhogs-mapviewer PRIVATE
    OpenHogs::content
    OpenHogs::render
    OpenHogs::extractor
    SDL3::SDL3)
openhogs_set_warnings(openhogs-mapviewer)

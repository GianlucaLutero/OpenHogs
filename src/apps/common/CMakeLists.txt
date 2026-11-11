# Support code shared by the SDL applications (map viewer, character
# viewer, sound test): opening the game files (GameFiles), the OpenGL
# window and screenshot saving (GlWindow).

add_library(openhogs_app_common STATIC
    src/GameFiles.cpp
    src/GlWindow.cpp)
add_library(OpenHogs::app_common ALIAS openhogs_app_common)

target_include_directories(openhogs_app_common PUBLIC include)
target_link_libraries(openhogs_app_common
    PUBLIC OpenHogs::core OpenHogs::formats
    PRIVATE OpenHogs::content
    PRIVATE OpenHogs::extractor SDL3::SDL3 glad)
openhogs_set_warnings(openhogs_app_common)

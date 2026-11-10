# formats: parsers for the file formats of the original game.
#
#   PackageArchive  - .MAD / .MTD archives (a list of named files)
#   GameText        - the game's 1-byte text encoding
#   TextTable       - .BIN + .OFS string tables (menus, mission names...)
#   TimImage        - PlayStation .TIM textures
#
# Format documentation lives in docs/formats/.

add_library(openhogs_formats STATIC
    src/GameText.cpp
    src/PackageArchive.cpp
    src/TextTable.cpp
    src/TimImage.cpp)

add_library(OpenHogs::formats ALIAS openhogs_formats)

target_include_directories(openhogs_formats PUBLIC include)
target_link_libraries(openhogs_formats PUBLIC OpenHogs::core)
openhogs_set_warnings(openhogs_formats)

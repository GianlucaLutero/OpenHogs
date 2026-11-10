# formats: parsers for the file formats of the original game.
#
#   PackageArchive  - .MAD / .MTD archives (a list of named files)
#   GameText        - the game's 1-byte text encoding
#   TextTable       - .BIN + .OFS string tables (menus, mission names...)
#   TimImage        - PlayStation .TIM textures
#   Model           - .VTX / .NO2 / .FAC 3D models
#   MapTerrain      - .PMG map terrain (heights and tiles)
#   MapObjects      - .POG objects placed on a map
#   TextureTable    - .PTG terrain textures
#
# Format documentation lives in docs/formats/.

add_library(openhogs_formats STATIC
    src/GameText.cpp
    src/MapObjects.cpp
    src/MapTerrain.cpp
    src/Model.cpp
    src/PackageArchive.cpp
    src/TextTable.cpp
    src/TextureTable.cpp
    src/TimImage.cpp)

add_library(OpenHogs::formats ALIAS openhogs_formats)

target_include_directories(openhogs_formats PUBLIC include)
target_link_libraries(openhogs_formats PUBLIC OpenHogs::core)
openhogs_set_warnings(openhogs_formats)

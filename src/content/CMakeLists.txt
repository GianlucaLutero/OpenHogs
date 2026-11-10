# content: everything the user can replace or switch on and off.
#
#   ContentConfig   - user settings from config/openhogs.ini
#   ContentPack     - one pack in content/packs/<id>/ described by pack.ini
#   ContentManager  - decides which packs are active and builds the
#                     VirtualFileSystem (game folder + active packs on top)

add_library(openhogs_content STATIC
    src/ContentConfig.cpp
    src/ContentPack.cpp
    src/ContentManager.cpp)

add_library(OpenHogs::content ALIAS openhogs_content)

target_include_directories(openhogs_content PUBLIC include)
target_link_libraries(openhogs_content PUBLIC OpenHogs::core)
openhogs_set_warnings(openhogs_content)

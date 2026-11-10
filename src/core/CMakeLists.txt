# core: small, dependency-free building blocks shared by every other module.
#
#   BinaryReader       - safe little-endian reading of the original file formats
#   IniFile            - reading of .ini configuration files
#   VirtualFileSystem  - layered, case-insensitive view over several folders

add_library(openhogs_core STATIC
    src/BinaryReader.cpp
    src/IniFile.cpp
    src/PathUtils.cpp
    src/VirtualFileSystem.cpp)

add_library(OpenHogs::core ALIAS openhogs_core)

target_include_directories(openhogs_core PUBLIC include)
openhogs_set_warnings(openhogs_core)

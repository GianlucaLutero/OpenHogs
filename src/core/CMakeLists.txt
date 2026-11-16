# core: small, dependency-free building blocks shared by every other module.
#
#   BinaryReader       - safe little-endian reading of the original file formats
#   IniFile            - reading of .ini configuration files
#   VirtualFileSystem  - layered, case-insensitive view over several folders
#   Version            - the OpenHogs version (generated from Version.h.in)

add_library(openhogs_core STATIC
    src/BinaryReader.cpp
    src/IniFile.cpp
    src/PathUtils.cpp
    src/VirtualFileSystem.cpp)

add_library(OpenHogs::core ALIAS openhogs_core)

# The version header is generated into the build folder.
configure_file(Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/openhogs/core/Version.h @ONLY)
target_include_directories(openhogs_core PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/generated)
openhogs_set_warnings(openhogs_core)

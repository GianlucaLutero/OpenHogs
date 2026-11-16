# Packaging: what a release contains and how it is zipped.
#
#   cmake --build build --config Release
#   cmake --install build --config Release --prefix <folder>   # the files only
#   cd build && cpack -C Release                                # the archive
#
# A release is one folder, used where it is unpacked:
#
#   OpenHogs-<version>-<system>/
#   ├── openhogs(.exe)            the game; the viewers and tools beside it
#   ├── config/openhogs.default.ini
#   │                             copied to openhogs.ini on the first run
#   ├── content/                  content packs and translations
#   ├── docs/                     file formats, architecture, open decisions
#   ├── licenses/                 the third-party licences
#   └── README.md, CHANGELOG.md, LICENSE
#
# The programs find config/ next to themselves, so they can be started by
# double-clicking. Nothing of the original game is in the package: it is
# read from the player's own installation.

set(OPENHOGS_PROGRAMS
    openhogs openhogs-mapviewer openhogs-characterviewer openhogs-soundtest
    openhogs-extract openhogs-text openhogs-langpack)
foreach(program IN LISTS OPENHOGS_PROGRAMS)
    if(TARGET ${program})
        install(TARGETS ${program} RUNTIME DESTINATION .)
    endif()
endforeach()

install(FILES config/openhogs.default.ini DESTINATION config)
install(DIRECTORY content/ DESTINATION content PATTERN ".gitkeep" EXCLUDE)
install(DIRECTORY docs/ DESTINATION docs)
install(FILES README.md CHANGELOG.md LICENSE DESTINATION .)

# Licences of the code built into the programs (see the README's Credits).
install(FILES third_party/README.md DESTINATION licenses RENAME THIRD_PARTY.md)
if(DEFINED sdl3_SOURCE_DIR AND EXISTS ${sdl3_SOURCE_DIR}/LICENSE.txt)
    install(FILES ${sdl3_SOURCE_DIR}/LICENSE.txt DESTINATION licenses RENAME SDL3.txt)
endif()
if(DEFINED glm_SOURCE_DIR AND EXISTS ${glm_SOURCE_DIR}/copying.txt)
    install(FILES ${glm_SOURCE_DIR}/copying.txt DESTINATION licenses RENAME GLM.txt)
endif()

# The archive: a .zip on Windows, a .tar.gz elsewhere.
set(CPACK_PACKAGE_NAME "OpenHogs")
set(CPACK_PACKAGE_VENDOR "OpenHogs contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(OPENHOGS_PACKAGE_SYSTEM "windows-x64")
else()
    set(CPACK_GENERATOR "TGZ")
    set(OPENHOGS_PACKAGE_SYSTEM "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    string(TOLOWER "${OPENHOGS_PACKAGE_SYSTEM}" OPENHOGS_PACKAGE_SYSTEM)
endif()
set(CPACK_PACKAGE_FILE_NAME "OpenHogs-${PROJECT_VERSION}-${OPENHOGS_PACKAGE_SYSTEM}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)

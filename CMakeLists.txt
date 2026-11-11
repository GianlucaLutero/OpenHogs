# OpenHogs - open-source reimplementation of Hogs of War.
#
# Top-level build script. Each module lives in its own sub-directory and
# builds a small static library; applications and tests link against them.

cmake_minimum_required(VERSION 3.24)

project(OpenHogs
    VERSION 0.1.0
    DESCRIPTION "Open-source reimplementation of Hogs of War"
    LANGUAGES C CXX)  # C only for the generated OpenGL loader (third_party/glad)

# --- Options ---------------------------------------------------------------
option(OPENHOGS_BUILD_TESTS "Build the unit test module" ON)
option(OPENHOGS_BUILD_VIEWER "Build the applications that need SDL3: map viewer, sound test (downloads and builds SDL3)" ON)

# --- Global settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put every executable in the same folder so tools and the game can find
# the shared `config/` and `content/` folders next to them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(OpenHogsWarnings)
include(OpenHogsDependencies)

# --- Modules ---------------------------------------------------------------
# Order matters: a module can only depend on modules added before it.
add_subdirectory(src/core)      # binary reading, INI files, virtual file system
add_subdirectory(src/content)   # replaceable content: packs, languages, config
add_subdirectory(src/formats)   # parsers for the original file formats
add_subdirectory(src/tools/extractor)  # openhogs-extract command-line tool
add_subdirectory(src/tools/text)       # openhogs-text: text tables <-> editable text
add_subdirectory(src/tools/langpack)   # openhogs-langpack: language packs from the PS1 disc
add_subdirectory(third_party/stb_vorbis) # Ogg Vorbis decoder (music)
add_subdirectory(src/audio)     # sound effects, speech, music: decoding and mixing
add_subdirectory(src/scene)     # map data turned into renderable geometry (no GPU code)

if(OPENHOGS_BUILD_VIEWER)
    add_subdirectory(third_party/glad)        # OpenGL function loader
    add_subdirectory(src/render)              # OpenGL renderer and camera
    add_subdirectory(src/apps/mapviewer)      # openhogs-mapviewer application
    add_subdirectory(src/apps/soundtest)      # openhogs-soundtest application
endif()

# --- Tests -----------------------------------------------------------------
if(OPENHOGS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

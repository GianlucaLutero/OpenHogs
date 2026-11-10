# Third-party libraries, downloaded by CMake at configure time.
#
# Each dependency is pinned to a release tag so that builds are repeatable.
# Authors and licenses are listed in the Credits section of the README.

include(FetchContent)

# GLM - vector and matrix maths (MIT license). Header-only.
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

# SDL3 - window, input and OpenGL context (zlib license).
# Built as a static library so the viewer is a single executable.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.20
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glm)

# SDL is only needed by the applications that open a window.
if(OPENHOGS_BUILD_VIEWER)
    FetchContent_MakeAvailable(SDL3)
endif()

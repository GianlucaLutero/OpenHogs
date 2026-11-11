# stb_vorbis - Ogg Vorbis decoder by Sean Barrett (v1.22).
#
# Source: https://github.com/nothings/stb (file stb_vorbis.c, unmodified)
# License: public domain or MIT, at your choice (see the end of the file).
# Used to decode the game's music (MUSIC/TrackNN.ogg). Third-party code, so
# the project warning level is not applied.

add_library(stb_vorbis STATIC stb_vorbis.c)
target_include_directories(stb_vorbis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Only memory decoding is used: no need for the stdio file API.
target_compile_definitions(stb_vorbis PUBLIC STB_VORBIS_NO_STDIO)
set_target_properties(stb_vorbis PROPERTIES LINKER_LANGUAGE C)
if(NOT MSVC)
    target_link_libraries(stb_vorbis PUBLIC m)
endif()

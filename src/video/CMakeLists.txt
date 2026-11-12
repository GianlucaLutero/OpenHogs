# video: the game's cutscenes, Bink 1 videos (Language/Streams/*.bik).
# Pure CPU decoding, no window or sound device, so it can be unit tested.
#
#   BitReader        - bits, least significant first
#   BinkFile         - the .bik container: header, frame table, packets
#   BinkVideoDecoder - pictures (YUV 4:2:0 planes, converted to RGBA)
#   BinkAudioDecoder - sound (the RDFT variant)
#   BinkTables       - constant tables of the codec (from FFmpeg, LGPL 2.1+)

add_library(openhogs_video STATIC
    src/BinkAudioDecoder.cpp
    src/BinkFile.cpp
    src/BinkTables.cpp
    src/BinkVideoDecoder.cpp
    src/BitReader.cpp)
add_library(OpenHogs::video ALIAS openhogs_video)

target_include_directories(openhogs_video PUBLIC include PRIVATE src)
target_link_libraries(openhogs_video PUBLIC OpenHogs::core OpenHogs::formats)
openhogs_set_warnings(openhogs_video)

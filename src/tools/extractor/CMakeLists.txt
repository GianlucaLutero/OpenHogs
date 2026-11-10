# extractor: command-line tool that unpacks the original game data into
# ordinary files (PNG images, text files, raw archive entries) so they can
# be inspected, edited or used as a base for content packs.
#
# The logic lives in a small library so that the unit tests can use it
# without going through the command line.

add_library(openhogs_extractor STATIC
    src/AssetExtractor.cpp
    src/PngWriter.cpp)

add_library(OpenHogs::extractor ALIAS openhogs_extractor)

target_include_directories(openhogs_extractor PUBLIC include)
target_link_libraries(openhogs_extractor PUBLIC OpenHogs::core OpenHogs::formats)
openhogs_set_warnings(openhogs_extractor)

add_executable(openhogs-extract src/main.cpp)
target_link_libraries(openhogs-extract PRIVATE OpenHogs::extractor)
openhogs_set_warnings(openhogs-extract)

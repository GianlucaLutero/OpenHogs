# Unit test module.
#
# Uses Catch2 v3, downloaded automatically by CMake the first time you
# configure the project. Run the tests with:
#
#     ctest --test-dir build --output-on-failure
#
# See tests/README.md for the rules every test must follow.

include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(openhogs_tests
    support/TempDirectory.cpp
    core/BinaryReaderTests.cpp
    core/IniFileTests.cpp
    core/PathUtilsTests.cpp
    core/VirtualFileSystemTests.cpp
    content/ContentConfigTests.cpp
    content/ContentPackTests.cpp
    content/ContentManagerTests.cpp
    formats/GameTextTests.cpp
    formats/PackageArchiveTests.cpp
    formats/TextTableTests.cpp
    formats/TimImageTests.cpp
    extractor/AssetExtractorTests.cpp
    extractor/PngWriterTests.cpp)

target_include_directories(openhogs_tests PRIVATE support)
target_link_libraries(openhogs_tests PRIVATE
    OpenHogs::core
    OpenHogs::content
    OpenHogs::formats
    OpenHogs::extractor
    Catch2::Catch2WithMain)
openhogs_set_warnings(openhogs_tests)

# Register every TEST_CASE with CTest individually.
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(openhogs_tests)

# langpack: builds a language content pack (content/packs/lang-<code>/)
# from the user's own PlayStation disc and PC installation.
#
#   TextAlignment         - moves PlayStation translations onto the PC tables
#   TranslationOverrides  - hand-written translations (content/translations/)
#   Bitmap                - .BMP reading/writing and bilinear scaling
#   LanguagePackBuilder   - puts it all together
#
# The logic is a library so the unit tests can use it directly.

add_library(openhogs_langpack STATIC
    src/Bitmap.cpp
    src/LanguagePackBuilder.cpp
    src/TextAlignment.cpp
    src/TranslationOverrides.cpp)

add_library(OpenHogs::langpack ALIAS openhogs_langpack)

target_include_directories(openhogs_langpack PUBLIC include)
target_link_libraries(openhogs_langpack PUBLIC OpenHogs::core OpenHogs::formats)
openhogs_set_warnings(openhogs_langpack)

add_executable(openhogs-langpack src/main.cpp)
target_link_libraries(openhogs-langpack PRIVATE OpenHogs::langpack)
openhogs_set_warnings(openhogs-langpack)

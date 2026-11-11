# openhogs-text: converts the game's text tables (.ofs + .bin) to readable,
# editable text files and back. The conversion itself lives in
# formats::TextTable; this is only the command-line front end.

add_executable(openhogs-text main.cpp)
target_link_libraries(openhogs-text PRIVATE OpenHogs::formats)
openhogs_set_warnings(openhogs-text)

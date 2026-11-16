// ConfigLocation.h
//
// Finds the user's configuration file, config/openhogs.ini.
//
// The programs look for it at a path relative to the current folder. A
// released package is used by double-clicking a program, or by starting
// it from any folder, so a relative path that does not exist there is
// also looked up next to the program itself.
//
// The package ships config/openhogs.default.ini but not openhogs.ini, so
// that unpacking a newer release never overwrites the user's settings.
// When openhogs.ini is missing and the default file is beside it, the
// default is copied to openhogs.ini: the game then starts at once for a
// Steam installation in its usual folder, and the user edits the copy for
// any other.

#pragma once

#include <filesystem>
#include <optional>

namespace openhogs::content {

struct ConfigLocation {
    std::filesystem::path path;   // the file to read (it may still not exist)
    bool createdFromDefault = false;  // openhogs.ini was just made from the default
};

// Resolves `requested` (as given on the command line or the default
// "config/openhogs.ini"): as it is if it exists, else relative to
// `programFolder` if it exists there; if neither exists, it is created
// from the openhogs.default.ini beside one of them (the current folder's
// first). Without either, `requested` comes back unchanged.
ConfigLocation locateConfig(const std::filesystem::path& requested,
                            const std::optional<std::filesystem::path>& programFolder);

} // namespace openhogs::content

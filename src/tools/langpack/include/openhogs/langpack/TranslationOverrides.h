// TranslationOverrides.h
//
// Hand-written translations that replace or complete the strings taken
// from the PlayStation disc. They live in content/translations/<lang>.ini
// and are original text written for OpenHogs (so they can be shared):
//
//     ; Tutorial lines: the PlayStation names its pad buttons,
//     ; the PC version needs the keyboard keys.
//     [gtext]
//     213 = PREMI SPAZIO PER SELEZIONARE L'ARMA.
//
//     [fetext]
//     460 = INSERISCI L'INDIRIZZO IP
//
// Keys are string indices in the PC tables; values are UTF-8 text in the
// syntax of formats::encodeGameText (accented capitals, {xx} icons).

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::langpack {

class TranslationOverrides {
public:
    static TranslationOverrides fromIni(const core::IniFile& ini);
    static TranslationOverrides load(const std::filesystem::path& path);

    // Replaces the entries of `table` ("gtext" or "fetext") listed in the
    // overrides. Returns how many entries were replaced. Throws
    // std::runtime_error if an index is outside the table, which usually
    // means the overrides were written for another version of the game.
    std::size_t apply(std::string_view table, std::vector<std::string>& entries) const;

    bool empty() const { return overrides_.empty(); }

private:
    // table name -> (index -> text)
    std::map<std::string, std::map<std::size_t, std::string>> overrides_;
};

} // namespace openhogs::langpack

// TextAlignment.h
//
// Moves a translation from the PlayStation text tables onto the PC tables.
//
// The PC and PlayStation versions share most strings, but not all of them:
// the PC menus have entries the PlayStation never had (network games, key
// names) and some credits differ. Their indices therefore drift apart (the
// PC front-end table has 786 strings, the PlayStation one 770).
//
// Both versions have an English table, so the English strings are used as
// anchors: the longest common subsequence of PC-English and
// PlayStation-English pairs every shared string, and each paired PC string
// takes the PlayStation translation at the paired index. PC strings without
// a pair stay in English and are reported, so they can be translated by
// hand (see TranslationOverrides).

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace openhogs::langpack {

struct AlignmentResult {
    // One string per PC entry: translated where a pair was found,
    // otherwise the PC English string.
    std::vector<std::string> entries;
    // PC indices left in English.
    std::vector<std::size_t> untranslated;
};

// Marker used by the PlayStation tables for strings that were never
// translated. Such entries are treated as missing and keep the PC English.
inline constexpr const char* kMissingTranslationMarker = "TEXT MISSING";

// `psxEnglish` and `psxTranslated` must have the same size (they are two
// languages of the same table). Throws std::invalid_argument otherwise.
AlignmentResult alignTranslation(const std::vector<std::string>& pcEnglish,
                                 const std::vector<std::string>& psxEnglish,
                                 const std::vector<std::string>& psxTranslated);

} // namespace openhogs::langpack

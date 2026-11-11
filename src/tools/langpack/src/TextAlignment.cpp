// TextAlignment.cpp - see TextAlignment.h for an overview.

#include "openhogs/langpack/TextAlignment.h"

#include <stdexcept>

namespace openhogs::langpack {

namespace {

// Classic dynamic-programming longest common subsequence. lengths[i][j] is
// the LCS length of a[i..] and b[j..]; walking the table from (0, 0) then
// yields the pairs. With ~800 x ~800 strings the table is small.
std::vector<std::pair<std::size_t, std::size_t>> longestCommonSubsequence(const std::vector<std::string>& a,
                                                                         const std::vector<std::string>& b) {
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    std::vector<std::vector<std::size_t>> lengths(n + 1, std::vector<std::size_t>(m + 1, 0));

    for (std::size_t i = n; i-- > 0;) {
        for (std::size_t j = m; j-- > 0;) {
            lengths[i][j] = (a[i] == b[j]) ? lengths[i + 1][j + 1] + 1
                                           : std::max(lengths[i + 1][j], lengths[i][j + 1]);
        }
    }

    std::vector<std::pair<std::size_t, std::size_t>> pairs;
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < n && j < m) {
        if (a[i] == b[j]) {
            pairs.emplace_back(i, j);
            ++i;
            ++j;
        } else if (lengths[i + 1][j] >= lengths[i][j + 1]) {
            ++i;
        } else {
            ++j;
        }
    }
    return pairs;
}

} // namespace

AlignmentResult alignTranslation(const std::vector<std::string>& pcEnglish,
                                 const std::vector<std::string>& psxEnglish,
                                 const std::vector<std::string>& psxTranslated) {
    if (psxEnglish.size() != psxTranslated.size()) {
        throw std::invalid_argument("PlayStation English and translated tables differ in size (" +
                                    std::to_string(psxEnglish.size()) + " vs " +
                                    std::to_string(psxTranslated.size()) + ")");
    }

    AlignmentResult result;
    result.entries = pcEnglish;  // start from English, then fill in translations
    std::vector<bool> translated(pcEnglish.size(), false);

    for (const auto& [pcIndex, psxIndex] : longestCommonSubsequence(pcEnglish, psxEnglish)) {
        if (psxTranslated[psxIndex] == kMissingTranslationMarker) {
            continue;
        }
        result.entries[pcIndex] = psxTranslated[psxIndex];
        translated[pcIndex] = true;
    }

    for (std::size_t i = 0; i < translated.size(); ++i) {
        if (!translated[i]) {
            result.untranslated.push_back(i);
        }
    }
    return result;
}

} // namespace openhogs::langpack

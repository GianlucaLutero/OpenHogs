// SpeechExtraction.cpp - see SpeechExtraction.h for an overview.

#include "openhogs/langpack/SpeechExtraction.h"

#include "openhogs/core/PathUtils.h"

#include <array>
#include <cctype>
#include <cstdio>

namespace openhogs::langpack {

namespace {

constexpr int kHogGroups = 5;
constexpr int kHogVariants = 6;
constexpr int kSargeSections = 22;
constexpr int kSargeVariants = 8;
constexpr int kChannelsPerFile = 32;

// Two decimal digits at `at` of `text`, or -1.
int twoDigits(std::string_view text, std::size_t at) {
    if (at + 2 > text.size()) return -1;
    if (!std::isdigit(static_cast<unsigned char>(text[at])) || !std::isdigit(static_cast<unsigned char>(text[at + 1]))) {
        return -1;
    }
    return (text[at] - '0') * 10 + (text[at + 1] - '0');
}

std::string twoDigitText(int number) {
    char text[8];
    std::snprintf(text, sizeof text, "%02d", number);
    return text;
}

// "speech/sku1/pig01/01en0203.wav" split into folder and file name.
struct SpeechPath {
    std::string folder;  // "pig01", lower case
    std::string name;    // "01en0203", lower case, without ".wav"
};

std::optional<SpeechPath> splitSpeechPath(std::string_view pcPath) {
    std::string path = core::toLower(std::string(pcPath));
    for (char& c : path) {
        if (c == '\\') c = '/';
    }
    constexpr std::string_view kPrefix = "speech/sku1/";
    constexpr std::string_view kSuffix = ".wav";
    if (path.rfind(kPrefix, 0) != 0 || path.size() < kPrefix.size() + kSuffix.size() ||
        path.compare(path.size() - kSuffix.size(), kSuffix.size(), kSuffix) != 0) {
        return std::nullopt;
    }
    const std::string rest = path.substr(kPrefix.size(), path.size() - kPrefix.size() - kSuffix.size());
    const auto slash = rest.find('/');
    if (slash == std::string::npos || rest.find('/', slash + 1) != std::string::npos) return std::nullopt;
    return SpeechPath{rest.substr(0, slash), rest.substr(slash + 1)};
}

std::optional<SpeechSource> hogLine(const SpeechPath& path) {
    // pigNN / NNAAGGVV
    if (path.folder.size() != 5 || path.folder.rfind("pig", 0) != 0 || path.name.size() != 8) return std::nullopt;
    const int voice = twoDigits(path.folder, 3);
    const int group = twoDigits(path.name, 4);
    const int variant = twoDigits(path.name, 6);
    if (voice < 1 || twoDigits(path.name, 0) != voice || group < 1 || group > kHogGroups || variant < 1 ||
        variant > kHogVariants) {
        return std::nullopt;
    }
    const std::string accent = core::toUpper(path.name.substr(2, 2));
    const std::string number = twoDigitText(voice);
    return SpeechSource{"SPEECH/SKU1/PIG" + number + "/" + number + accent + "01.IXA",
                        (group - 1) * kHogVariants + variant - 1};
}

std::optional<SpeechSource> sargeLine(const SpeechPath& path) {
    // sarge / sgenSSVV
    if (path.folder != "sarge" || path.name.size() != 8 || path.name.rfind("sgen", 0) != 0) return std::nullopt;
    const int section = twoDigits(path.name, 4);
    const int variant = twoDigits(path.name, 6);
    if (section < 1 || section > kSargeSections || variant < 1 || variant > kSargeVariants) return std::nullopt;
    const int line = (section - 1) * kSargeVariants + variant - 1;
    return SpeechSource{"SPEECH/SKU1/SARGE/SGEN" + twoDigitText(line / kChannelsPerFile + 1) + ".IXA",
                        line % kChannelsPerFile};
}

std::optional<SpeechSource> trainingLine(const SpeechPath& path) {
    // train1 / tr1_enNN
    if (path.folder != "train1" || path.name.size() != 8 || path.name.rfind("tr1_en", 0) != 0) return std::nullopt;
    const int line = twoDigits(path.name, 6);
    if (line < 1 || line > kChannelsPerFile) return std::nullopt;
    return SpeechSource{"SPEECH/SKU1/TRAIN1/TR1_EN.IXA", line - 1};
}

} // namespace

std::optional<SpeechSource> discSourceOf(std::string_view pcPath) {
    const auto path = splitSpeechPath(pcPath);
    if (!path) return std::nullopt;
    if (auto source = hogLine(*path)) return source;
    if (auto source = sargeLine(*path)) return source;
    return trainingLine(*path);
}

bool isDubbedRelease(std::string_view systemCnf) {
    const std::string text = core::toUpper(std::string(systemCnf));
    constexpr std::array<std::string_view, 4> kDubbed{"SLES_027.66", "SLES_027.67", "SLES_027.68", "SLES_027.69"};
    for (const auto id : kDubbed) {
        if (text.find(id) != std::string::npos) return true;
    }
    return false;
}

} // namespace openhogs::langpack

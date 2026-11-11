// CharacterLibrary.cpp - see CharacterLibrary.h for an overview.

#include "openhogs/scene/CharacterLibrary.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/PackageArchive.h"

#include <array>
#include <optional>
#include <stdexcept>

namespace openhogs::scene {

namespace {

// Names checked on the Steam release. The class names come from the
// texture names in the team archives (ACEU = ace uniform, GRUN = grunt,
// HEVY = heavy weapons, LEGE = legend, MEDI = medic, SAPP = sapper,
// SABO = saboteur, SNIP = sniper, SPYU = spy).
constexpr std::array<CharacterClass, 9> kClasses{{
    {"ace", "Ace", "pcace", "ac"},
    {"grunt", "Grunt", "pcgru", "gr"},
    {"heavy", "Heavy Weapons", "pchvy", "hv"},
    {"legend", "Legend", "pcleg", "le"},
    {"medic", "Medic", "pcmed", "me"},
    {"sapper", "Sapper", "pcsap", "sa"},
    {"saboteur", "Saboteur", "pcsab", "sb", true},
    {"sniper", "Sniper", "pcsni", "sn"},
    {"spy", "Spy", "pcspy", "sp"},
}};

constexpr std::array<std::string_view, 7> kTeams{"british", "american", "french", "german",
                                                 "japanese", "russian", "teamlard"};

// The animation names stored in warhogs_.exe (a table of 59 string
// pointers, in this order). Kept verbatim, including the developers' jokes
// ("Lord Flash-Heart", "Swimming like Rick").
constexpr std::array<std::string_view, 59> kAnimationNames{
    "Run cycle (normal)",
    "Run cycle (wounded)",
    "Run cycle (more wounded)",
    "Walk cycle (backwards)",
    "Turning on Spot",
    "Swimming",
    "Swimming like Rick",
    "Getting into Vehicles",
    "Jumping - Start",
    "Jumping - Middle",
    "Jumping - End",
    "Scramble",
    "Getting out Handgun",
    "Getting out Rifle",
    "Getting out Machine gun",
    "Getting out Heavy weapon",
    "Getting out Punch",
    "Getting out Grenade",
    "Getting out Sword / Knife",
    "Using Grenade",
    "Using Punch",
    "Sword / Knife",
    "Bayonet",
    "Aiming Handgun",
    "Aiming Rifle",
    "Aiming Machine gun",
    "Aiming Heavy weapon",
    "Standing around cycle 1",
    "Standing around cycle 2",
    "Very Wounded",
    "Lord Flash-Heart Pose",
    "Looking around",
    "Looking gormless",
    "Cowering",
    "Brushoff 1",
    "Brushoff 2",
    "Brushoff 3",
    "Sneeze",
    "Flying through air/falling",
    "Bouncing on B-Hind",
    "Getting to feet",
    "Celebration #1",
    "Celebration #2",
    "Celebration #3",
    "Salute",
    "Look back",
    "Thinking",
    "Dying #1",
    "Dying #2",
    "Dying #3",
    "Drowning",
    "Idle Cold",
    "Idle Hot",
    "Lay Mine",
    "Heal",
    "Pick pocket",
    "Air strike",
    "Hari Kiri",
    "Parachuting",
};

constexpr std::size_t kHogBoneCount = 15;

const char* const kSkeletonPath = "Chars/pig.HIR";
const char* const kAnimationsPath = "Chars/mcap.mad";
const char* const kModelsPath = "Chars/british.mad";
const char* const kFacesPath = "Chars/FACES.MTD";
const char* const kExpressionRulesPath = "Chars/expressions.ini";

} // namespace

std::span<const CharacterClass> characterClasses() {
    return kClasses;
}

std::span<const std::string_view> characterTeams() {
    return kTeams;
}

const CharacterClass* findCharacterClass(std::string_view id) {
    const std::string key = core::makeLookupKey(id);
    for (const auto& characterClass : kClasses) {
        if (characterClass.id == key) {
            return &characterClass;
        }
    }
    return nullptr;
}

std::string_view animationName(std::size_t animation) {
    return animation < kAnimationNames.size() ? kAnimationNames[animation] : std::string_view();
}

std::string characterModelName(const CharacterClass& characterClass, CharacterDetail detail) {
    switch (detail) {
    case CharacterDetail::High:
        return std::string(characterClass.pcModel) + "_hi";
    case CharacterDetail::Medium:
        return std::string(characterClass.pcModel) + "_me";
    case CharacterDetail::Low:
        break;
    }
    return std::string(characterClass.lowModel) + "_hi";
}

CharacterLibrary::CharacterLibrary(const core::VirtualFileSystem& vfs)
    : vfs_(vfs),
      skeleton_(formats::Skeleton::parse(vfs.readFile(kSkeletonPath))),
      motion_(formats::MotionCapture::parse(vfs.readFile(kAnimationsPath), skeleton_.bones.size())),
      expressionRules_(ExpressionRules::defaults()) {
    if (vfs.exists(kExpressionRulesPath)) {
        const auto data = vfs.readFile(kExpressionRulesPath);
        expressionRules_.applyOverrides(
            core::IniFile::parse(std::string_view(reinterpret_cast<const char*>(data.data()), data.size())));
    }
    if (skeleton_.bones.size() != kHogBoneCount) {
        // The models' bone indices are made for this skeleton.
        throw std::runtime_error(std::string(kSkeletonPath) + " has " + std::to_string(skeleton_.bones.size()) +
                                 " bones, expected " + std::to_string(kHogBoneCount));
    }
}

formats::Model CharacterLibrary::loadModel(const CharacterClass& characterClass, CharacterDetail detail) const {
    const auto archive = formats::PackageArchive::parse(vfs_.readFile(kModelsPath));
    const std::string name = characterModelName(characterClass, detail);

    const auto* vtx = archive.find(name + ".vtx");
    const auto* fac = archive.find(name + ".fac");
    if (vtx == nullptr || fac == nullptr) {
        throw std::runtime_error(std::string(kModelsPath) + " has no model " + name);
    }
    const auto* no2 = archive.find(name + ".no2");
    const auto normals = no2 ? std::optional(archive.entryData(*no2)) : std::nullopt;
    return formats::Model::parse(archive.entryData(*vtx), archive.entryData(*fac), normals);
}

CharacterTextures CharacterLibrary::loadTeamTextures(std::string_view team) const {
    CharacterTextures result;
    std::optional<std::uint32_t> defaultEyes;
    std::optional<std::uint32_t> defaultMouth;

    // Adds every texture of an archive. Face textures are recognised by
    // name: "eyes00N" / "gobs00N", N = 0 for the team's default face.
    const auto addArchive = [&](const std::string& path) {
        const auto archive = formats::PackageArchive::parse(vfs_.readFile(path));
        for (const auto& entry : archive.entries()) {
            const auto index = static_cast<std::uint32_t>(result.images.size());
            result.images.push_back(formats::decodeTim(archive.entryData(entry)));

            const std::string name = core::toLower(entry.name);
            if (name.size() < 8 || (!name.starts_with("eyes") && !name.starts_with("gobs")) || name[4] != '0' ||
                name[5] != '0' || name[6] < '0' || name[6] > '7') {
                continue;
            }
            const auto number = static_cast<std::size_t>(name[6] - '0');
            const bool isEyes = name.starts_with("eyes");
            if (number == 0) {
                (isEyes ? defaultEyes : defaultMouth) = index;
            } else {
                (isEyes ? result.faces.eyes : result.faces.mouths)[number] = index;
            }
        }
    };

    addArchive("Chars/" + std::string(team) + ".mtd");
    if (!defaultEyes || !defaultMouth) {
        return result;  // no face to change: hogs keep their default one
    }
    result.faces.defaultEyes = defaultEyes;
    result.faces.defaultMouth = defaultMouth;
    if (vfs_.exists(kFacesPath)) {
        addArchive(kFacesPath);
    }
    return result;
}

std::vector<glm::uvec2> textureSizes(const std::vector<formats::RgbaImage>& textures) {
    std::vector<glm::uvec2> sizes;
    for (const auto& texture : textures) {
        sizes.emplace_back(texture.width, texture.height);
    }
    return sizes;
}

} // namespace openhogs::scene

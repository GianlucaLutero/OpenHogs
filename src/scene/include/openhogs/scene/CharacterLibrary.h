// CharacterLibrary.h
//
// Loads the hogs from the game's Chars/ folder (see
// docs/formats/characters.md):
//
//     Chars/pig.HIR        the skeleton shared by every hog
//     Chars/mcap.mad       the 93 animations
//     Chars/british.mad    the body models of every class, in three levels
//                          of detail (pcace_hi, pcace_me, ac_hi...)
//     Chars/<team>.mtd     the textures of one team (british, american...),
//                          all teams using the same models
//     Chars/FACES.MTD      extra eyes and mouths for facial expressions
//     Chars/WEAPONS.MAD    weapon models (grenades, guns, TNT...), with
//     Chars/WEAPONS.MTD    their textures
//     Chars/expressions.ini  optional: replaces the expression rules
//                          (only in content packs; the game has no such file)
//
// Files are read through the virtual file system, so a content pack can
// replace any of them (a new team skin is just another .mtd).

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/Model.h"
#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/scene/CharacterExpressions.h"

#include <glm/glm.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::scene {

// The hog classes of the game, with the names used in the model files.
struct CharacterClass {
    std::string_view id;        // used on the command line, e.g. "ace"
    std::string_view name;      // display name
    std::string_view pcModel;   // PC models: <pcModel>_hi and <pcModel>_me
    std::string_view lowModel;  // simplest model: <lowModel>_hi
    bool masked = false;        // face covered by a black balaclava (saboteur)
};

// Level of detail of a body model. The PC release has two detailed models
// per class ("pc..._hi", "pc..._me"); the third is a much simpler model
// (about 200 vertices instead of 650) with its own "..LO" textures.
enum class CharacterDetail { High, Medium, Low };

// The nine classes that have a body model.
std::span<const CharacterClass> characterClasses();

// Team texture sets: Chars/<name>.mtd.
std::span<const std::string_view> characterTeams();

// Finds a class by id (case-insensitive); nullptr if there is none.
const CharacterClass* findCharacterClass(std::string_view id);

// Name of a body model in british.mad, e.g. ("ace", High) -> "pcace_hi".
std::string characterModelName(const CharacterClass& characterClass, CharacterDetail detail);

// Name of an animation, as listed in the game's executable (an internal
// debug list, e.g. "Run cycle (normal)", "Salute"). The list has 59 names
// for the first 59 animations; the other 34 have no name and get an empty
// string. The match was checked by looking at the animations (0 is a run,
// 5 swimming, 30 hands on hips, 44 a salute...).
std::string_view animationName(std::size_t animation);

// Speed of the animations in mcap.mad, checked against the original game.
inline constexpr float kAnimationFramesPerSecond = 25.0f;

// How far below the model origin a standing hog's feet are, in model units
// (Y points down in model space). Measured on the idle animations: the
// lowest point of the body stays about 133-150 units below the origin.
inline constexpr float kStandingFeetDepth = 145.0f;

// The textures a hog is drawn with: the team archive followed by the
// expression textures of FACES.MTD.
struct CharacterTextures {
    std::vector<formats::RgbaImage> images;  // index = texture index in meshes
    FaceTextureSlots faces;
};

class CharacterLibrary {
public:
    // Reads the skeleton and the animations. Throws if they are missing or
    // malformed. The file system must outlive the library.
    explicit CharacterLibrary(const core::VirtualFileSystem& vfs);

    const formats::Skeleton& skeleton() const { return skeleton_; }
    const std::vector<formats::Animation>& animations() const { return motion_.animations; }

    // Loads one body model. Throws if the model is missing or malformed.
    formats::Model loadModel(const CharacterClass& characterClass, CharacterDetail detail) const;

    // Decodes the textures of a team, in archive order (the order FAC
    // texture indices count), followed by the expression textures. Throws
    // if the team file is missing; without FACES.MTD or without the
    // default eyes and mouth, hogs simply keep their default face.
    CharacterTextures loadTeamTextures(std::string_view team) const;

    // Loads a weapon model of Chars/WEAPONS.MAD by name (e.g. "WE_GREN").
    // Throws if it is missing.
    formats::Model loadWeaponModel(std::string_view name) const;

    // Decodes the weapon textures, in archive order. Throws if missing.
    std::vector<formats::RgbaImage> loadWeaponTextures() const;

    // Which expression goes with which animation: the built-in rules, with
    // Chars/expressions.ini applied when a content pack provides one.
    const ExpressionRules& expressionRules() const { return expressionRules_; }

private:
    const core::VirtualFileSystem& vfs_;
    formats::Skeleton skeleton_;
    formats::MotionCapture motion_;
    ExpressionRules expressionRules_;
};

// Width and height of each texture, as buildModelMesh() expects them.
std::vector<glm::uvec2> textureSizes(const std::vector<formats::RgbaImage>& textures);

} // namespace openhogs::scene

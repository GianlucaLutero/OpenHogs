// CharacterExpressions.h
//
// Facial expressions of the hogs.
//
// A hog's eyes and mouth are two small textures on the front of the head.
// Every body model draws them with the team archive's default textures
// (eyes000 and gobs000, the last two entries of <team>.mtd). The game ships
// seven more of each in Chars/FACES.MTD, and changing the expression just
// means drawing those faces with another texture (see
// docs/formats/characters.md):
//
//   eyes001 open      eyes002 closed     eyes003 sad      eyes004 wide open
//   eyes005 angry     eyes006/007 open/closed, on black (the saboteur's balaclava)
//   gobs001 grin      gobs002 neutral    gobs003 frown    gobs004 "O" (shock)
//   gobs005 gritted teeth   gobs006/007 grin/neutral, on black
//
// Which expression the original game shows in which situation is not known
// yet (it is decided in code, not data). Until it is, OpenHogs picks one
// from the animation being played, with rules that follow the animation
// names; the rules can be replaced by a file (Chars/expressions.ini) in a
// content pack. On top of the expression the hog blinks every few seconds.

#pragma once

#include "openhogs/core/IniFile.h"
#include "openhogs/scene/Geometry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace openhogs::scene {

enum class Expression {
    Neutral,     // open eyes, closed mouth
    Happy,       // open eyes, grin
    Sad,         // sad eyes, frown (hurt, cold, tired)
    Shocked,     // wide eyes, "O" mouth (falling, cowering, drowning)
    Angry,       // angry eyes, gritted teeth (fighting, aiming)
    EyesClosed,  // closed eyes, neutral mouth (sneezing)
    Dying,       // closed eyes, "O" mouth
    Gormless,    // the default face of the team archive: raised brows, tongue out
};

// Name used in expressions.ini, e.g. "angry".
std::string_view expressionName(Expression expression);
std::optional<Expression> parseExpression(std::string_view name);

// Expression for each animation, by number.
class ExpressionRules {
public:
    // The built-in rules (see the .cpp for the table and its reasons).
    static ExpressionRules defaults();

    // Replaces rules with the [Animations] section of an expressions.ini:
    //     [Animations]
    //     0 = neutral
    //     44 = happy
    // Throws core::FormatError for an unknown expression or a key that is
    // not an animation number.
    void applyOverrides(const core::IniFile& ini);

    // Neutral for animations without a rule.
    Expression forAnimation(std::size_t animation) const;

private:
    std::vector<Expression> byAnimation_;
};

// True during the short moments the hog has its eyes closed. Each hog
// blinks for 0.15 s once every 5.5 seconds on average, at irregular
// moments; `seed` (e.g. the hog's number) keeps hogs from blinking all at
// the same time. The schedule is a
// pure function of time, so it needs no state and is the same every run.
bool isBlinking(double seconds, std::uint32_t seed);

// Where the face textures are in a character's texture list.
struct FaceTextureSlots {
    // The indices the models use (eyes000, gobs000); empty when the team
    // archive has no default face, and then expressions do nothing.
    std::optional<std::uint32_t> defaultEyes;
    std::optional<std::uint32_t> defaultMouth;
    // Index of eyes00N / gobs00N for N = 1..7 ([0] is unused); missing
    // textures stay empty, and the default face is drawn instead.
    std::array<std::optional<std::uint32_t>, 8> eyes{};
    std::array<std::optional<std::uint32_t>, 8> mouths{};
};

// Changes the texture of the eye and mouth triangles of a posed mesh to
// show `expression`. `masked` selects the black variants (saboteur);
// `blinking` closes the eyes whatever the expression.
void applyExpression(Mesh& mesh, const FaceTextureSlots& slots, Expression expression, bool blinking, bool masked);

} // namespace openhogs::scene

// HogMovement.h
//
// How a hog walks, turns, jumps, falls and swims, one simulation step at a
// time.
//
// The player's controls arrive as a MovementInput; moveHog() turns them
// into a new position, the matching animation and the events the rest of
// the game reacts to (footsteps, landing, splashing into water).
//
// The animations of the original stay on the spot (the run cycle does not
// move the hips forward), so the game moves the hog itself. The running
// speed comes from the run cycle's stride, so the feet do not slide.
// Every other value (jump height, turning speed...) is OpenHogs' own
// choice until the original's are known; they are listed in
// docs/open-decisions.md.

#pragma once

#include "openhogs/formats/MapTerrain.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/MapCollision.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

namespace openhogs::game {

// Animation numbers used for movement (names in scene::animationName).
inline constexpr std::size_t kRunAnimation = 0;           // "Run cycle (normal)", 17 frames
inline constexpr std::size_t kWalkBackAnimation = 3;      // "Walk cycle (backwards)"
inline constexpr std::size_t kTurnAnimation = 4;          // "Turning on Spot"
inline constexpr std::size_t kSwimAnimation = 6;          // "Swimming like Rick": with nothing in hand (the executable uses 5 only with a weapon out)
inline constexpr std::size_t kJumpStartAnimation = 8;     // "Jumping - Start"
inline constexpr std::size_t kJumpMiddleAnimation = 9;    // "Jumping - Middle"
inline constexpr std::size_t kJumpEndAnimation = 10;      // "Jumping - End"
inline constexpr std::size_t kFallAnimation = 38;         // "Flying through air/falling"
inline constexpr std::size_t kFirstDyingAnimation = 47;   // "Dying #1", #2 and #3 follow
inline constexpr std::size_t kDrowningAnimation = 74;     // sinking: the executable's death in water

// Speeds in world units per second.
// The run cycle's feet travel 326 model units (163 world units) per step,
// two steps per 17-frame cycle at 25 frames per second: 480 units/s.
inline constexpr float kRunSpeed = 480.0f;
// The backward walk's feet travel 143 model units in 20 frames: 180 units/s.
inline constexpr float kWalkBackSpeed = 180.0f;
inline constexpr float kSwimSpeedFactor = 0.5f;   // in water
inline constexpr float kTurnDegreesPerSecond = 150.0f;
inline constexpr float kGravity = 2400.0f;        // units/s^2
inline constexpr float kJumpUpSpeed = 900.0f;     // apex about 170 units above the take-off
inline constexpr float kJumpForwardSpeed = kRunSpeed;

// What the player asks the hog to do during one step.
struct MovementInput {
    bool forward = false;
    bool backward = false;
    bool turnLeft = false;
    bool turnRight = false;
    bool jump = false;  // only acted on when the hog stands on the ground
    // The hog the player controls stays inside the play area: the
    // executable stops it kPlayAreaHalfSize from the centre of the map,
    // one block short of the edge ("Blocked by world limits").
    bool keepInPlayArea = false;
};

// Half the side of the square the controlled hog may move in, centred on
// the map (world units; the map itself is 16 blocks of 2048 = 32768 wide).
inline constexpr float kPlayAreaHalfSize = 12288.0f;

// What happened during the step, for sounds and the turn rules.
struct MovementEvents {
    float distanceMoved = 0.0f;  // horizontal, world units
    int footsteps = 0;
    bool tookOff = false;        // a jump started
    bool landed = false;
    float fallHeight = 0.0f;     // when landed: from the highest point to the ground
    bool enteredWater = false;
    formats::SurfaceType surface = formats::SurfaceType::Grass;  // under the hog
};

// Advances the hog `hogIndex` of `hogs` by one step of `seconds`. Other
// hogs are obstacles. `canMove` is false when the hog has no movement left
// this turn: it can still turn on the spot, but not walk or jump.
MovementEvents moveHog(std::vector<Hog>& hogs, std::size_t hogIndex, const MovementInput& input, bool canMove,
                       double seconds, const MapCollision& map, const std::vector<formats::Animation>& animations);

// Name (without extension) of the footstep sound for a surface in the
// game's sound bank Audio/sfxday.srl: FT_GRASS, FT_SNOW, FT_WATER...
// The bank has one for each of the twelve surface types.
std::string_view footstepSoundName(formats::SurfaceType surface);

// True if the hog stands in water (the tile under it is a water tile).
bool isInWater(const Hog& hog, const MapCollision& map);

} // namespace openhogs::game

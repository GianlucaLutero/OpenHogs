// SteamPuffs.h - the puffs of steam the original's menu machines blow out.
//
// While the list menu's panel drops fast, and while the longer list rises,
// the executable sets off little steam jets at the machines' pipes
// (FUN_00423e70 creates FUN_00419fa0 objects): each jet blows four puffs a
// tick for eight ticks (FUN_0041a0b0); a puff drifts with its speed,
// jitters by up to two pixels a tick, sinks a little faster each tick and
// fades out after eight ticks (FUN_0041a140), shrinking and darkening from
// a warm white (FUN_0041a1d0, FUN_0041a280). The first jet also starts the
// steam sound.
//
// Positions are pixels of the 640 x 480 screen; nothing here draws.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openhogs::ui {

class SteamPuffs {
public:
    struct Puff {
        float x = 0.0f, y = 0.0f;
        float size = 0.0f;       // pixels across
        float red = 0.0f, green = 0.0f, blue = 0.0f;  // 0..1
    };

    // A jet at (x, y) blowing with speed (dx, dy) pixels a tick.
    void blow(int x, int y, int dx, int dy);

    void tick();
    bool empty() const { return jets_.empty() && particles_.empty(); }
    void clear();

    // Returns true once after the first jet of a burst: the steam sound.
    bool takeHiss();

    std::vector<Puff> puffs() const;

private:
    struct Jet {
        int x, y, dx, dy;
        int age = 0;
    };
    struct Particle {
        int x, y, dx, dy;
        int age = 0;
    };

    int below(int count);   // 0 .. count - 1
    int jitter(int range);  // -range .. range

    std::vector<Jet> jets_;
    std::vector<Particle> particles_;
    std::uint32_t random_ = 20261123u;
    bool hiss_ = false;
};

} // namespace openhogs::ui

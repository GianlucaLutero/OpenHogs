// SteamPuffs.cpp - see SteamPuffs.h for an overview.

#include "openhogs/ui/SteamPuffs.h"

#include <algorithm>

namespace openhogs::ui {

namespace {

constexpr int kJetTicks = 8;           // a jet blows for eight ticks...
constexpr int kPuffsPerTick = 4;       // ...four puffs at a time
constexpr int kSpread = 4;             // puffs start up to 4 pixels and 4 pixels a tick off
constexpr int kLastAge = 7;            // a puff lives eight ticks
constexpr std::size_t kMaxPuffs = 32;  // per jet in the executable; plenty for all of them

} // namespace

void SteamPuffs::blow(int x, int y, int dx, int dy) {
    if (jets_.empty() && particles_.empty()) hiss_ = true;
    jets_.push_back({x, y, dx, dy});
}

void SteamPuffs::clear() {
    jets_.clear();
    particles_.clear();
}

bool SteamPuffs::takeHiss() {
    const bool hiss = hiss_;
    hiss_ = false;
    return hiss;
}

int SteamPuffs::below(int count) {
    // A small linear congruential generator: the puffs only need to look random.
    random_ = random_ * 1103515245u + 12345u;
    return static_cast<int>((random_ >> 16) % static_cast<std::uint32_t>(count));
}

int SteamPuffs::jitter(int range) { return below(2 * range + 1) - range; }

void SteamPuffs::tick() {
    // The puffs move, wobble, sink and age (FUN_00419AC0, FUN_0041A140).
    for (auto& puff : particles_) {
        puff.x += puff.dx;
        puff.y += puff.dy;
        ++puff.age;
        // (random & 3) - 2: -2 .. 1 pixels.
        puff.x += below(4) - 2;
        puff.y += below(4) - 2;
        puff.dy += 1;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) { return p.age > kLastAge; }),
                     particles_.end());
    // The jets blow their puffs (FUN_0041A0B0).
    for (auto& jet : jets_) {
        for (int i = 0; i < kPuffsPerTick && particles_.size() < kMaxPuffs * 8; ++i) {
            particles_.push_back({jet.x + jitter(kSpread), jet.y + jitter(kSpread), jet.dx + jitter(kSpread),
                                  jet.dy + jitter(kSpread)});
        }
        ++jet.age;
    }
    jets_.erase(std::remove_if(jets_.begin(), jets_.end(), [](const Jet& jet) { return jet.age >= kJetTicks; }),
                jets_.end());
}

std::vector<SteamPuffs::Puff> SteamPuffs::puffs() const {
    std::vector<Puff> result;
    result.reserve(particles_.size());
    for (const auto& particle : particles_) {
        const float age = static_cast<float>(particle.age);
        Puff puff;
        puff.x = static_cast<float>(particle.x);
        puff.y = static_cast<float>(particle.y);
        // FUN_0041A280: (2048 - 128 × age) × 2 / 1024, doubled when drawn.
        puff.size = (2048.0f - 128.0f * age) * 2.0f / 1024.0f * 2.0f;
        // FUN_0041A1D0: from (127, 120, 50) out of 128, darker with age.
        puff.red = (127.0f - 5.875f * age) / 128.0f;
        puff.green = (120.0f - 13.75f * age) / 128.0f;
        puff.blue = (50.0f - 6.25f * age) / 128.0f;
        result.push_back(puff);
    }
    return result;
}

} // namespace openhogs::ui

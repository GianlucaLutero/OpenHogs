// CampaignMap.cpp - see CampaignMap.h for an overview.

#include "openhogs/ui/CampaignMap.h"

#include <algorithm>
#include <cmath>

namespace openhogs::ui {

int mapIslandOfMission(int mission) {
    const int island = std::clamp((mission - 1) / 5, 0, 4);
    // The fifth group of five holds Arstria's four missions and the last
    // one, which has an island of its own (FUN_00483980).
    if (island == 4 && mission >= kMapTerritories) return 5;
    return island;
}

MissionRange mapIslandMissions(int island) {
    // FUN_004833b0: five missions an island, Arstria four, the Isle of
    // Swill one.
    switch (island) {
    case 4: return {21, 24};
    case 5: return {25, 25};
    default: return {island * 5 + 1, island * 5 + 5};
    }
}

CampaignMapAnimation::CampaignMapAnimation(int mission)
    : mission_(std::clamp(mission, 1, kMapTerritories)) {}

int CampaignMapAnimation::ticks() const {
    return static_cast<int>(std::floor(phaseSeconds_ / kMapTickSeconds + 1e-9));
}

void CampaignMapAnimation::update(double seconds) {
    if (done()) return;
    phaseSeconds_ += seconds;
    const auto next = [this](CampaignMapPhase phase) {
        phase_ = phase;
        phaseSeconds_ = 0.0;
    };
    switch (phase_) {
    case CampaignMapPhase::Map:
        if (phaseSeconds_ >= kMapSeconds) next(CampaignMapPhase::Zoom);
        break;
    case CampaignMapPhase::Zoom:
        if (ticks() >= kZoomTicks) next(CampaignMapPhase::Island);
        break;
    case CampaignMapPhase::Island:
        if (phaseSeconds_ >= kIslandSeconds) next(CampaignMapPhase::Done);
        break;
    case CampaignMapPhase::Done:
        break;
    }
}

void CampaignMapAnimation::keyPressed() {
    if (phase_ == CampaignMapPhase::Map) {
        phase_ = CampaignMapPhase::Zoom;
        phaseSeconds_ = 0.0;
    } else if (phase_ == CampaignMapPhase::Island) {
        phase_ = CampaignMapPhase::Done;
    }
}

bool CampaignMapAnimation::territoryFlashing() const {
    if (phase_ != CampaignMapPhase::Map) return false;
    // The executable tests bit 4 of its millisecond clock: white for 16 ms,
    // coloured for 16 ms.
    return static_cast<int>(std::floor(phaseSeconds_ / kFlickerSeconds)) % 2 == 1;
}

float CampaignMapAnimation::zoomProgress() const {
    if (phase_ == CampaignMapPhase::Map) return 0.0f;
    if (phase_ != CampaignMapPhase::Zoom) return 1.0f;
    return static_cast<float>(kZoomCurve[static_cast<std::size_t>(std::min(ticks(), kZoomTicks - 1))]) / 100.0f;
}

int CampaignMapAnimation::flagsShown() const {
    if (phase_ == CampaignMapPhase::Done) {
        const auto range = mapIslandMissions(island());
        return range.last - range.first + 1;
    }
    if (phase_ != CampaignMapPhase::Island) return 0;
    const auto range = mapIslandMissions(island());
    // FUN_004833b0 plants the flags one every three ticks, the first on
    // the second tick.
    return std::min(range.last - range.first + 1, (ticks() + 1) / 3);
}

bool CampaignMapAnimation::arrowsShown() const {
    const auto range = mapIslandMissions(island());
    return phase_ == CampaignMapPhase::Island && flagsShown() == range.last - range.first + 1;
}

int CampaignMapAnimation::arrowBounce() const {
    // The step climbs 0..5 and comes back down, one step a tick.
    const int step = ticks() % 10;
    return kArrowBounce[static_cast<std::size_t>(step <= 5 ? step : 10 - step)];
}

} // namespace openhogs::ui

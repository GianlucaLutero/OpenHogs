// CampaignMenu.cpp - see CampaignMenu.h for an overview.

#include "openhogs/ui/CampaignMenu.h"

#include "openhogs/ui/TextLayout.h"

#include <utility>

namespace openhogs::ui {

namespace {

using Type = MenuResponse::Type;
using Request = CampaignMenuResult::Request;

// Item ids. Hogs use their place in the squad (0..7); promotions use the
// rank's number.
constexpr int kStartMission = 100;
constexpr int kBack = 101;
constexpr int kPromote = 102;
constexpr int kSwap = 103;
constexpr int kContinue = 104;
constexpr int kRetry = 105;
constexpr int kEditSquad = 106;

MenuItem action(int id, std::string label, bool enabled = true, std::string detail = {}) {
    MenuItem item;
    item.id = id;
    item.label = std::move(label);
    item.enabled = enabled;
    item.detail = std::move(detail);
    return item;
}

bool canPromoteAtAll(const game::Campaign& campaign, std::size_t hog) {
    for (const auto rank : game::promotionsFrom(campaign.squad()[hog].rank)) {
        if (campaign.canPromote(hog, rank)) return true;
    }
    return false;
}

MenuSound soundFor(const MenuResponse& response) {
    switch (response.type) {
    case Type::Moved:
    case Type::Changed: return MenuSound::Move;
    case Type::Activated: return MenuSound::Select;
    case Type::Back: return MenuSound::Back;
    case Type::None: break;
    }
    return MenuSound::None;
}

} // namespace

CampaignMenu::CampaignMenu(UiStrings strings, DeploymentRule deployment)
    : strings_(std::move(strings)), deployment_(std::move(deployment)) {}

std::string CampaignMenu::pointsLine(int points) const {
    return fillPlaceholders(strings_.promotionPoints, {}, {points});
}

void CampaignMenu::showSquad(const game::Campaign& campaign) {
    screen_ = CampaignScreen::Squad;
    deployed_ = deployment_ ? deployment_(campaign) : campaign.deploymentSize(game::kDeployedHogs);
    const std::size_t deployed = deployed_;
    lines_.clear();
    if (!campaign.finished()) {
        const auto& mission = game::campaignMission(campaign.nextMission());
        lines_.push_back(fillPlaceholders(strings_.missionTitle, {strings_.missionName(mission)}, {mission.number}));
    }
    lines_.push_back(pointsLine(campaign.points()));

    std::vector<MenuItem> items;
    const auto& squad = campaign.squad();
    for (std::size_t i = 0; i < squad.size(); ++i) {
        // "*" marks the hogs that go on the next mission.
        const std::string marker = i < deployed ? "* " : "";
        items.push_back(action(static_cast<int>(i), marker + squad[i].name, true, strings_.rankName(squad[i].rank)));
    }
    items.push_back(action(kStartMission, strings_.startMission, !campaign.finished()));
    items.push_back(action(kBack, strings_.back));
    menu_ = Menu(strings_.teamName(campaign.nation()), std::move(items));
    if (!campaign.finished()) menu_.select(kStartMission);
}

void CampaignMenu::showHog(const game::Campaign& campaign) {
    screen_ = CampaignScreen::Hog;
    const auto& member = campaign.squad()[hog_];
    lines_ = {strings_.rankName(member.rank), pointsLine(campaign.points())};
    menu_ = Menu(member.name, {action(kPromote, strings_.promote, canPromoteAtAll(campaign, hog_)),
                               action(kSwap, strings_.swapPosition), action(kBack, strings_.back)});
}

void CampaignMenu::showPromotion(const game::Campaign& campaign) {
    screen_ = CampaignScreen::Promotion;
    const auto& member = campaign.squad()[hog_];
    lines_ = {member.name + " - " + strings_.rankName(member.rank), pointsLine(campaign.points())};
    const bool choosingCareer = game::rankSpec(member.rank).career == game::Career::None;

    std::vector<MenuItem> items;
    for (const auto rank : game::promotionsFrom(member.rank)) {
        const auto& spec = game::rankSpec(rank);
        // A grunt chooses a career: show its name, with the rank it starts at.
        std::string label = strings_.rankName(rank);
        if (choosingCareer && spec.career >= game::Career::HeavyWeapons && spec.career <= game::Career::Engineer) {
            label = strings_.careers[static_cast<std::size_t>(spec.career) - 1] + " - " + label;
        }
        items.push_back(action(static_cast<int>(rank), label, campaign.canPromote(hog_, rank),
                               std::to_string(game::promotionCost(rank))));
    }
    items.push_back(action(kBack, strings_.back));
    menu_ = Menu(choosingCareer ? strings_.careerPath : strings_.promote, std::move(items));
}

void CampaignMenu::showSwap(const game::Campaign& campaign) {
    screen_ = CampaignScreen::Swap;
    lines_ = {campaign.squad()[hog_].name};
    std::vector<MenuItem> items;
    const auto& squad = campaign.squad();
    for (std::size_t i = 0; i < squad.size(); ++i) {
        if (i == hog_) continue;
        const std::string marker = i < deployed_ ? "* " : "";
        items.push_back(action(static_cast<int>(i), marker + squad[i].name, true, strings_.rankName(squad[i].rank)));
    }
    items.push_back(action(kBack, strings_.back));
    menu_ = Menu(strings_.swapPosition, std::move(items));
}

void CampaignMenu::showResults(const game::Campaign& campaign, const MissionSummary& summary) {
    screen_ = CampaignScreen::Results;
    const auto& outcome = summary.outcome;
    resultsCompleted_ = outcome.completed;
    campaignWon_ = campaign.finished();

    const auto& mission = game::campaignMission(summary.missionNumber);
    lines_ = {fillPlaceholders(strings_.missionTitle, {strings_.missionName(mission)}, {mission.number})};
    if (summary.aborted) {
        lines_.push_back(strings_.missionAborted);
    } else if (!outcome.completed) {
        lines_.push_back(strings_.missionFailed);
    } else {
        lines_.push_back(strings_.levelComplete);
        if (outcome.survivalBonus) lines_.push_back(strings_.survivalBonus);
        for (const auto& name : outcome.lostForGood) {
            lines_.push_back(strings_.rip + " " + name);
        }
        lines_.push_back("+" + std::to_string(outcome.pointsEarned) + "  " + pointsLine(campaign.points()));
        if (campaignWon_) lines_.push_back(strings_.campaignComplete);
    }

    std::vector<MenuItem> items;
    if (outcome.completed) {
        items.push_back(action(kContinue, strings_.continueGame));
    } else {
        items.push_back(action(kRetry, strings_.retry));
        items.push_back(action(kEditSquad, strings_.editSquad));
        items.push_back(action(kBack, strings_.back));
    }
    menu_ = Menu(strings_.missionResults, std::move(items));
}

CampaignMenuResult CampaignMenu::handle(MenuInput input, game::Campaign& campaign) {
    const auto response = menu_.handle(input);
    switch (screen_) {
    case CampaignScreen::Squad: return handleSquad(response, campaign);
    case CampaignScreen::Hog: return handleHog(response, campaign);
    case CampaignScreen::Promotion: return handlePromotion(response, campaign);
    case CampaignScreen::Swap: return handleSwap(response, campaign);
    case CampaignScreen::Results: return handleResults(response, campaign);
    }
    return {};
}

CampaignMenuResult CampaignMenu::handleSquad(const MenuResponse& response, game::Campaign& campaign) {
    CampaignMenuResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kBack)) {
        result.request = Request::Leave;
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated && response.itemId == kStartMission) {
        result.request = Request::StartMission;
    } else if (response.type == Type::Activated) {
        hog_ = static_cast<std::size_t>(response.itemId);
        showHog(campaign);
    }
    return result;
}

CampaignMenuResult CampaignMenu::handleHog(const MenuResponse& response, game::Campaign& campaign) {
    CampaignMenuResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kBack)) {
        showSquad(campaign);
        menu_.select(static_cast<int>(hog_));
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated && response.itemId == kPromote) {
        showPromotion(campaign);
    } else if (response.type == Type::Activated && response.itemId == kSwap) {
        showSwap(campaign);
    }
    return result;
}

CampaignMenuResult CampaignMenu::handlePromotion(const MenuResponse& response, game::Campaign& campaign) {
    CampaignMenuResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kBack)) {
        showHog(campaign);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        if (campaign.promote(hog_, static_cast<game::RankId>(response.itemId))) {
            result.request = Request::Changed;
        }
        showSquad(campaign);
        menu_.select(static_cast<int>(hog_));
    }
    return result;
}

CampaignMenuResult CampaignMenu::handleSwap(const MenuResponse& response, game::Campaign& campaign) {
    CampaignMenuResult result;
    result.sound = soundFor(response);
    if (response.type == Type::Back || (response.type == Type::Activated && response.itemId == kBack)) {
        showHog(campaign);
        result.sound = MenuSound::Back;
    } else if (response.type == Type::Activated) {
        const auto other = static_cast<std::size_t>(response.itemId);
        campaign.swap(hog_, other);
        result.request = Request::Changed;
        showSquad(campaign);
        menu_.select(static_cast<int>(other));
    }
    return result;
}

CampaignMenuResult CampaignMenu::handleResults(const MenuResponse& response, game::Campaign& campaign) {
    CampaignMenuResult result;
    result.sound = soundFor(response);
    if (response.type != Type::Activated && response.type != Type::Back) {
        return result;
    }
    const int chosen = response.type == Type::Back ? (resultsCompleted_ ? kContinue : kBack) : response.itemId;
    switch (chosen) {
    case kContinue:
        if (campaignWon_) {
            result.request = Request::Leave;
        } else {
            showSquad(campaign);
        }
        break;
    case kRetry: result.request = Request::StartMission; break;
    case kEditSquad: showSquad(campaign); break;
    case kBack:
        result.request = Request::Leave;
        result.sound = MenuSound::Back;
        break;
    default: break;
    }
    return result;
}

} // namespace openhogs::ui

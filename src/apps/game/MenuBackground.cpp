// MenuBackground.cpp - see MenuBackground.h for an overview.

#include "MenuBackground.h"

#include "openhogs/scene/MapScene.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace openhogs::gameapp {

namespace {

constexpr float kDegreesPerSecond = 3.0f;  // one lap in two minutes
constexpr float kRadiusShare = 0.45f;      // of the map's width
constexpr float kHeightShare = 0.25f;      // above the map's highest point, of its width

} // namespace

MenuBackground::MenuBackground(const core::VirtualFileSystem& vfs, const std::string& mapName) {
    const auto scene = scene::loadMapScene(vfs, mapName);
    renderer_ = std::make_unique<render::MapRenderer>(scene);
    centre_ = (scene.boundsMin + scene.boundsMax) / 2.0f;
    const float size = std::max(scene.boundsMax.x - scene.boundsMin.x, scene.boundsMax.z - scene.boundsMin.z);
    radius_ = size * kRadiusShare;
    height_ = scene.boundsMax.y - centre_.y + size * kHeightShare;
}

void MenuBackground::update(double seconds) {
    angleDegrees_ = std::fmod(angleDegrees_ + kDegreesPerSecond * static_cast<float>(seconds), 360.0f);
}

void MenuBackground::draw(int width, int height) const {
    const float angle = glm::radians(angleDegrees_);
    render::Camera camera;
    camera.position = centre_ + glm::vec3(std::sin(angle) * radius_, height_, std::cos(angle) * radius_);
    // Look at the centre: yaw 0 looks towards -Z (see render::Camera).
    const glm::vec3 along = centre_ - camera.position;
    camera.yawDegrees = glm::degrees(std::atan2(along.x, -along.z));
    camera.pitchDegrees = glm::degrees(std::atan2(along.y, std::hypot(along.x, along.z)));

    render::DrawOptions options;
    options.showMarkers = false;
    renderer_->draw(camera, width, height, options);
}

} // namespace openhogs::gameapp

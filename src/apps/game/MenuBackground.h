// MenuBackground.h
//
// What is behind the menus: a map seen from a camera that slowly circles
// above it, darkened so the menu stands out.
//
// The original shows full-screen pictures here (FEBmps/*.mgl), but their
// compression is not decoded yet (docs/open-decisions.md); a live map is a
// fitting stand-in that needs nothing new.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/render/MapRenderer.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace openhogs::gameapp {

class MenuBackground {
public:
    // Loads the map; needs a current OpenGL context. Throws if the map
    // cannot be loaded.
    MenuBackground(const core::VirtualFileSystem& vfs, const std::string& mapName);

    void update(double seconds);
    void draw(int width, int height) const;

private:
    std::unique_ptr<render::MapRenderer> renderer_;
    glm::vec3 centre_{0.0f};
    float radius_ = 0.0f;
    float height_ = 0.0f;
    float angleDegrees_ = 0.0f;
};

} // namespace openhogs::gameapp

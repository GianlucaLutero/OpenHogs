// MenuModels.h
//
// The 3D models the original's menus show over their pictures:
//
//   - the hog on the pedestal of CAREER PATH (the executable stands the
//     hog's model on `standpc` in the information board's monitor), here
//     in the body of the career being chosen, standing and slowly turning;
//   - the medals of the debriefing page: the promotion point model
//     (Chars/PROPOINT.MAD, propoint.mtd) turning in place
//     (FUN_004845e0, FUN_00480bc0).
//
// They are drawn after the menu's pictures, into the 640 x 480 area in the
// middle of the window, with the original's camera: focal length 480 pixels
// at 640 x 480, looking into the screen (the executable places the medals
// at depth 7000).
//
// Needs a current OpenGL context. The models and textures load on first use.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/Model.h"
#include "openhogs/render/CharacterRenderer.h"
#include "openhogs/scene/CharacterLibrary.h"
#include "openhogs/scene/Geometry.h"

#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>

namespace openhogs::gameapp {

class MenuModels {
public:
    explicit MenuModels(const core::VirtualFileSystem& vfs);
    ~MenuModels();

    // Where the 640 x 480 screen of the original lies in the window: its
    // left edge in canvas units and the window's size in pixels.
    struct Screen {
        float originX = 0.0f;  // canvas units (the canvas is 480 units high)
        int windowWidth = 0;
        int windowHeight = 0;
    };

    // A hog of class `classId` in the colours of `nation` (scene::characterTeams
    // order), its feet at `feet` (pixels of the 640 x 480 screen), `height`
    // pixels tall, turned `yawRadians`, `seconds` into its standing cycle.
    void drawHog(const Screen& screen, std::string_view classId, int nation, glm::vec2 feet, float height,
                 float yawRadians, double seconds);

    // The medal at the executable's place `position` (x right, y up, depth
    // into the screen), turned `turn` (0..4095, a whole turn), `opacity` 0..1.
    void drawMedal(const Screen& screen, const glm::vec3& position, int turn, float opacity);

private:
    struct TeamLook {
        std::unique_ptr<render::CharacterTextureSet> textures;
        std::vector<glm::uvec2> textureSizes;
        scene::FaceTextureSlots faces;
    };

    bool begin(const Screen& screen);  // sets the viewport; false if nothing can be drawn
    void end();
    scene::CharacterLibrary* library();
    const TeamLook* look(int nation);
    const formats::Model* hogModel(std::string_view classId);
    bool loadMedal();

    const core::VirtualFileSystem& vfs_;
    std::unique_ptr<render::CharacterRenderer> renderer_;
    std::unique_ptr<scene::CharacterLibrary> library_;
    bool libraryFailed_ = false;
    std::map<int, TeamLook> looks_;
    std::map<std::string, formats::Model, std::less<>> models_;
    std::optional<scene::Mesh> medalMesh_;
    std::unique_ptr<render::CharacterTextureSet> medalTextures_;
    bool medalFailed_ = false;
    int viewportWidth_ = 0, viewportHeight_ = 0;
};

} // namespace openhogs::gameapp

// MenuModels.cpp - see MenuModels.h for an overview.

#include "MenuModels.h"

#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/game/IdleAnimator.h"
#include "openhogs/render/Camera.h"
#include "openhogs/scene/CharacterExpressions.h"
#include "openhogs/scene/CharacterPose.h"
#include "openhogs/scene/ModelMesh.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr float kScreenWidth = 640.0f, kScreenHeight = 480.0f;
// The original's camera: 480 pixels of focal length on a 480-pixel-high
// screen, so a vertical field of view of 2 × atan(240 / 480).
constexpr float kFocalPixels = 480.0f;
const float kFieldOfViewDegrees = glm::degrees(2.0f * std::atan(kScreenHeight / 2.0f / kFocalPixels));
// The hog stands this far into the screen; its size on screen comes from it.
constexpr float kHogDepth = 1600.0f;
constexpr float kHogModelHeight = 570.0f;
constexpr float kAnimationRate = 25.0f;  // the characters' animations, frames a second
constexpr double kTwoPi = 6.283185307179586;

const char* const kMedalArchive = "Chars/PROPOINT.MAD";
const char* const kMedalTextures = "Chars/propoint.mtd";
const char* const kMedalModel = "propoint";

// A point of the original screen (pixels, y down) at `depth`, as a world
// point seen by the menu camera (at the origin, looking towards -Z, y up).
glm::vec3 worldAt(glm::vec2 pixel, float depth) {
    return {(pixel.x - kScreenWidth / 2.0f) * depth / kFocalPixels, (kScreenHeight / 2.0f - pixel.y) * depth / kFocalPixels,
            -depth};
}

} // namespace

MenuModels::MenuModels(const core::VirtualFileSystem& vfs) : vfs_(vfs) {}

MenuModels::~MenuModels() = default;

scene::CharacterLibrary* MenuModels::library() {
    if (!library_ && !libraryFailed_) {
        try {
            library_ = std::make_unique<scene::CharacterLibrary>(vfs_);
        } catch (const std::exception& error) {
            std::cerr << "Warning: no hog models for the menus: " << error.what() << "\n";
            libraryFailed_ = true;
        }
    }
    return library_.get();
}

const MenuModels::TeamLook* MenuModels::look(int nation) {
    auto found = looks_.find(nation);
    if (found != looks_.end()) return &found->second;
    auto* characters = library();
    const auto teams = scene::characterTeams();
    if (!characters || nation < 0 || static_cast<std::size_t>(nation) >= teams.size()) return nullptr;
    try {
        auto textures = characters->loadTeamTextures(teams[static_cast<std::size_t>(nation)]);
        TeamLook look;
        look.textureSizes = scene::textureSizes(textures.images);
        look.faces = textures.faces;
        look.textures = std::make_unique<render::CharacterTextureSet>(textures.images);
        return &looks_.emplace(nation, std::move(look)).first->second;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no textures for team " << teams[static_cast<std::size_t>(nation)] << ": " << error.what() << "\n";
        return nullptr;
    }
}

const formats::Model* MenuModels::hogModel(std::string_view classId) {
    auto found = models_.find(classId);
    if (found != models_.end()) return &found->second;
    auto* characters = library();
    const auto* characterClass = scene::findCharacterClass(classId);
    if (!characters || !characterClass) return nullptr;
    try {
        return &models_.emplace(std::string(classId), characters->loadModel(*characterClass, scene::CharacterDetail::High))
                    .first->second;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no model for " << classId << ": " << error.what() << "\n";
        return nullptr;
    }
}

bool MenuModels::loadMedal() {
    if (medalMesh_) return true;
    if (medalFailed_) return false;
    try {
        const auto archive = formats::PackageArchive::parse(vfs_.readFile(kMedalArchive));
        const auto* vtx = archive.find(std::string(kMedalModel) + ".vtx");
        const auto* fac = archive.find(std::string(kMedalModel) + ".fac");
        if (!vtx || !fac) throw std::runtime_error("no propoint model");
        const auto* no2 = archive.find(std::string(kMedalModel) + ".no2");
        const auto normals = no2 ? std::optional(archive.entryData(*no2)) : std::nullopt;
        const auto model = formats::Model::parse(archive.entryData(*vtx), archive.entryData(*fac), normals);
        // The textures in the order of propoint.mtd: the model's indices.
        const auto textureArchive = formats::PackageArchive::parse(vfs_.readFile(kMedalTextures));
        std::vector<formats::RgbaImage> images;
        for (const auto& entry : textureArchive.entries()) images.push_back(formats::decodeTim(textureArchive.entryData(entry)));
        medalMesh_ = scene::buildModelMesh(model, scene::textureSizes(images));
        medalTextures_ = std::make_unique<render::CharacterTextureSet>(images);
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no medal model for the debriefing: " << error.what() << "\n";
        medalFailed_ = true;
        return false;
    }
}

bool MenuModels::begin(const Screen& screen) {
    if (screen.windowWidth <= 0 || screen.windowHeight <= 0) return false;
    if (!renderer_) renderer_ = std::make_unique<render::CharacterRenderer>();
    // The canvas is 480 units high: one unit is windowHeight / 480 pixels.
    const float pixelsPerUnit = static_cast<float>(screen.windowHeight) / kScreenHeight;
    const int left = static_cast<int>(std::lround(screen.originX * pixelsPerUnit));
    viewportWidth_ = static_cast<int>(std::lround(kScreenWidth * pixelsPerUnit));
    viewportHeight_ = screen.windowHeight;
    glViewport(left, 0, viewportWidth_, viewportHeight_);
    // The models stand in front of every picture of the menu.
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, 0, viewportWidth_, viewportHeight_);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    return true;
}

void MenuModels::end() { glDisable(GL_DEPTH_TEST); }

void MenuModels::drawHog(const Screen& screen, std::string_view classId, int nation, glm::vec2 feet, float height,
                         float yawRadians, double seconds) {
    const auto* model = hogModel(classId);
    const auto* team = look(nation);
    auto* characters = library();
    if (!model || !team || !characters || !begin(screen)) return;

    const auto& animations = characters->animations();
    const auto& standing = animations.at(game::kStandingAnimation);
    const float frames = static_cast<float>(std::max<std::size_t>(standing.frames.size(), 1));
    const float frame = std::fmod(static_cast<float>(seconds) * kAnimationRate, frames);
    const auto pose = scene::samplePose(characters->skeleton(), standing, frame);
    auto mesh = scene::buildPosedModelMesh(*model, team->textureSizes, pose);
    const auto* characterClass = scene::findCharacterClass(classId);
    scene::applyExpression(mesh, team->faces, characters->expressionRules().forAnimation(game::kStandingAnimation),
                           scene::isBlinking(seconds, 7u), characterClass && characterClass->masked);

    // The hog's height on screen sets its size (measured on the drawn
    // models: about 570 model units from the feet to the tips of the ears).
    const float scale = height * kHogDepth / kFocalPixels / kHogModelHeight;
    const glm::vec3 feetPoint = worldAt(feet, kHogDepth);
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), feetPoint + glm::vec3(0.0f, scene::kStandingFeetDepth * scale, 0.0f));
    transform = glm::rotate(transform, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(scale, -scale, scale));  // model y is down

    render::Camera camera;
    camera.fieldOfViewDegrees = kFieldOfViewDegrees;
    renderer_->drawHog(camera, viewportWidth_, viewportHeight_, mesh, *team->textures, transform);
    end();
}

void MenuModels::drawMedal(const Screen& screen, const glm::vec3& position, int turn, float opacity) {
    if (!loadMedal() || !begin(screen)) return;
    // FUN_00480bc0: turned about the upright axis, model y down, one model
    // unit to one unit of the executable's scene.
    const float angle = static_cast<float>((turn % 4096) * kTwoPi / 4096.0);
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, -position.z));
    transform = glm::rotate(transform, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(1.0f, -1.0f, 1.0f));

    render::Camera camera;
    camera.fieldOfViewDegrees = kFieldOfViewDegrees;
    renderer_->drawHog(camera, viewportWidth_, viewportHeight_, *medalMesh_, *medalTextures_, transform,
                       glm::vec4(1.0f, 1.0f, 1.0f, opacity));
    end();
}

} // namespace openhogs::gameapp

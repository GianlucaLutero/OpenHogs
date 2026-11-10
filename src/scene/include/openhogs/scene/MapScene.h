// MapScene.h
//
// Everything needed to draw one map, loaded from the virtual file system:
//
//     Maps/<NAME>.PMG   terrain             -> terrain mesh
//     Maps/<NAME>.PTG   terrain textures    -> terrainTextures
//     Maps/<NAME>.MAD   object models       -> models
//     Maps/<NAME>.MTD   object textures     -> modelTextures
//     Maps/<NAME>.POG   object placement    -> objects and markers
//
// Because the files come from the virtual file system, content packs can
// replace any of them (e.g. new textures) without touching the game.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/scene/Geometry.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::scene {

// One placed copy of a model.
struct ModelInstance {
    std::string modelName;  // key into MapScene::models
    glm::mat4 transform{1.0f};  // model units -> world units
};

struct MapScene {
    std::string name;

    Mesh terrain;
    std::vector<formats::RgbaImage> terrainTextures;  // index = PTG texture index

    std::map<std::string, Mesh> models;              // by upper-case model name
    std::vector<formats::RgbaImage> modelTextures;   // index = FAC texture index
    std::vector<ModelInstance> objects;

    // Positions of objects that have no model, such as the start positions
    // of the hogs (GR_ME, CO_ME...). The viewer draws them as markers.
    std::vector<glm::vec3> markers;

    // Axis-aligned bounds of the terrain, used to place the camera.
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    // Non-fatal problems (a broken texture, a model that failed to load).
    std::vector<std::string> warnings;
};

// Converts an object's position and rotation into a model -> world matrix.
glm::mat4 objectTransform(const formats::MapObject& object);

// Loads a map by name ("CAMP", case-insensitive). Throws std::runtime_error
// if the terrain itself is missing or invalid; smaller problems end up in
// MapScene::warnings so the map can still be shown.
MapScene loadMapScene(const core::VirtualFileSystem& vfs, std::string_view mapName);

// Upper-case names of all maps that have a terrain file, sorted.
std::vector<std::string> listMaps(const core::VirtualFileSystem& vfs);

} // namespace openhogs::scene

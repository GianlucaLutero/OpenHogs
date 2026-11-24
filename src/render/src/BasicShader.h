// BasicShader.h (private to the render module)
//
// The shader shared by the map and character renderers.
//
// Texture colour times the vertex brightness (baked or computed lighting)
// times a tint colour (used for markers and the ground grid), faded into
// the fog colour with the distance from the eye (see Fog.h; uFogRange.x is
// the start, .y the end, and an end not past the start means no fog).
// Fully transparent texels are discarded: that is how the game cuts out
// tree leaves, fences and similar shapes.

#pragma once

#include "openhogs/render/Fog.h"
#include "openhogs/render/GlObjects.h"

namespace openhogs::render {

inline constexpr const char* kBasicVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in float inBrightness;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec2 uv;
out float brightness;
out vec3 worldPosition;

void main() {
    vec4 world = uModel * vec4(inPosition, 1.0);
    gl_Position = uViewProjection * world;
    uv = inUv;
    brightness = inBrightness;
    worldPosition = world.xyz;
}
)";

inline constexpr const char* kBasicFragmentShader = R"(
#version 330 core
in vec2 uv;
in float brightness;
in vec3 worldPosition;

uniform sampler2D uTexture;
uniform vec4 uTint;
uniform vec4 uEye;
uniform vec4 uFogColour;
uniform vec4 uFogRange;

out vec4 outColor;

void main() {
    vec4 texel = texture(uTexture, uv);
    if (texel.a < 0.5) {
        discard;
    }
    outColor = vec4(texel.rgb * brightness, 1.0) * uTint;
    if (uFogRange.y > uFogRange.x) {
        float fog = clamp((distance(worldPosition, uEye.xyz) - uFogRange.x) / (uFogRange.y - uFogRange.x), 0.0, 1.0);
        outColor.rgb = mix(outColor.rgb, uFogColour.rgb, fog);
    }
}
)";

// Sets the fog uniforms of a program built from the shaders above.
inline void applyFog(const GlShaderProgram& shader, const Fog& fog, const glm::vec3& eye) {
    shader.setVector("uEye", glm::vec4(eye, 1.0f));
    shader.setVector("uFogColour", glm::vec4(fog.colour, 1.0f));
    shader.setVector("uFogRange", glm::vec4(fog.start, fog.end, 0.0f, 0.0f));
}

} // namespace openhogs::render

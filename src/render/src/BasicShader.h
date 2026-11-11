// BasicShader.h (private to the render module)
//
// The shader shared by the map and character renderers.
//
// Texture colour times the vertex brightness (baked or computed lighting)
// times a tint colour (used for markers and the ground grid). Fully
// transparent texels are discarded: that is how the game cuts out tree
// leaves, fences and similar shapes.

#pragma once

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

void main() {
    gl_Position = uViewProjection * uModel * vec4(inPosition, 1.0);
    uv = inUv;
    brightness = inBrightness;
}
)";

inline constexpr const char* kBasicFragmentShader = R"(
#version 330 core
in vec2 uv;
in float brightness;

uniform sampler2D uTexture;
uniform vec4 uTint;

out vec4 outColor;

void main() {
    vec4 texel = texture(uTexture, uv);
    if (texel.a < 0.5) {
        discard;
    }
    outColor = vec4(texel.rgb * brightness, 1.0) * uTint;
}
)";


} // namespace openhogs::render

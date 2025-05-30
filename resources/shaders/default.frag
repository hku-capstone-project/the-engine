#version 450

// Texture sampler for the base color
// This corresponds to _images.baseColor in your Renderer.cpp
layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;

// Varyings received from the vertex shader
layout(location = 0) in vec2 fragTexCoord; // Texture coordinates
layout(location = 1) in vec3 fragNormal;   // World-space normal

// Final output color
layout(location = 0) out vec4 outColor;

void main() {
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 textureColor    = texture(baseColorSampler, flippedTexCoord);

    // For now, just output the texture color
    outColor = textureColor;
}

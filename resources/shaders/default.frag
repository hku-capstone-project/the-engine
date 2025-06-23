#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec3 vertWorldPosition;

layout(location = 0) out vec4 outColor;

#include "include/sharedVariables.glsl"
layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; }
renderInfo;

layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;

void main() {
    // vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    // vec4 textureColor    = texture(baseColorSampler, flippedTexCoord);

    // based on vertColor, but darken it as it goes away from the camera
    vec3 cameraPosition = vec3(0.0, 0.0, 0.0);
    
    vec3 camPos = renderInfo.data.view[3].xyz;
    float distance = length(camPos - vertWorldPosition);
    float intensity = 1.0 - distance / 10.0;
    vec3 color = vertColor * intensity;

    outColor = vec4(color, 1.0);
}

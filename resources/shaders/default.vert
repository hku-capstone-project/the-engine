#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 viewPos;
layout(location = 4) out vec4 fragTangent;

#include "include/sharedVariables.glsl"

layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; } renderInfo;

void main() {
    vec4 worldPos = renderInfo.data.model * vec4(inPos, 1.0);
    float dist = length(worldPos);

    gl_Position = renderInfo.data.proj * renderInfo.data.view * worldPos;
    fragPos = worldPos.xyz;
    fragTexCoord = inTexCoord;
    fragNormal = normalize(mat3(transpose(inverse(renderInfo.data.model))) * inNormal);
    viewPos = renderInfo.data.viewPos;
    fragTangent = inTangent;
}

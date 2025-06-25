#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPos;      // from Vertex::pos
layout(location = 1) in vec2 inTexCoord; // from Vertex::texCoord
layout(location = 2) in vec3 inNormal;   // from Vertex::normal
layout(location = 3) in vec4 inTangent;  // Available if needed

// layout(location = 0) out vec2 fragTexCoord; // Pass texture coordinates
// layout(location = 1) out vec3 fragNormal;   // Pass normal (e.g., for lighting)
layout(location = 0) out vec3 vertColor;
layout(location = 1) out vec3 vertWorldPosition;

#include "include/sharedVariables.glsl"
layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; }
renderInfo;

// Push constants for per-object model matrix
layout(push_constant) uniform ModelMatrix {
    mat4 model;
} pushConstants;

void main() {
    vec4 modelSpacePosition = vec4(inPos, 1.0);

    // Use push constants for model matrix instead of uniform buffer
    gl_Position =
        renderInfo.data.proj * renderInfo.data.view * pushConstants.model * modelSpacePosition;

    vertWorldPosition = (pushConstants.model * modelSpacePosition).xyz;

    vertColor = vec3(1.0, 0.0, 0.0);

    // fragTexCoord = inTexCoord;
    // fragNormal   = normalize(mat3(transpose(inverse(renderInfo.data.model))) *
    //                          inNormal); // Transform normal to world space
}

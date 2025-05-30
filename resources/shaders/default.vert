#version 450

#extension GL_GOOGLE_include_directive : require

#include "include/sharedVariables.glsl"

// Uniform Buffer Object for transformation matrices
// This should match the structure of G_RenderInfo in your C++ code
layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; }
renderInfo;

// Vertex inputs, matching the C++ Vertex struct and Vulkan setup
layout(location = 0) in vec3 inPos;      // from Vertex::pos
layout(location = 1) in vec2 inTexCoord; // from Vertex::texCoord
layout(location = 2) in vec3 inNormal;   // from Vertex::normal
layout(location = 3) in vec4 inTangent;  // Available if needed

// Varyings passed to the fragment shader
layout(location = 0) out vec2 fragTexCoord; // Pass texture coordinates
layout(location = 1) out vec3 fragNormal;   // Pass normal (e.g., for lighting)

void main() {
    // Transform vertex position from model space to clip space
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);

    // If using a pre-combined MVP matrix:
    // gl_Position = ubo.mvp * vec4(inPos, 1.0);

    // For simplicity, if your G_RenderInfo only contains a single MVP matrix:
    // Assume G_RenderInfo is: struct G_RenderInfo { glm::mat4 mvp; };
    // Then the UBO would be:
    // layout(set = 0, binding = 0) uniform UboMVP {
    //     mat4 mvp;
    // } uboMVP;
    // gl_Position = uboMVP.mvp * vec4(inPos, 1.0);

    // Using separate model, view, proj matrices:
    gl_Position = renderInfo.data.proj * renderInfo.data.view * renderInfo.data.model * vec4(inPos, 1.0);

    // gl_Position = vec4(inPos, 1.0);

    // Pass attributes to the fragment shader
    fragTexCoord = inTexCoord;
    // fragNormal   = normalize(mat3(transpose(inverse(ubo.model))) *
    //                          inNormal); // Transform normal to world space
    fragNormal = inNormal; // Pass normal directly if no transformation needed
}

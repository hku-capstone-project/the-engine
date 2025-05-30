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
    gl_Position =
        renderInfo.data.proj * renderInfo.data.view * renderInfo.data.model * vec4(inPos, 1.0);

    fragTexCoord = inTexCoord;
    fragNormal   = normalize(mat3(transpose(inverse(renderInfo.data.model))) *
                             inNormal); // Transform normal to world space
}

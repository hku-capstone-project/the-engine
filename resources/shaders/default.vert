#version 450

#extension GL_GOOGLE_include_directive : require

#include "include/sharedVariables.glsl"

// Uniform Buffer Object for transformation matrices
layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; }
renderInfo;

// 摄像机位置（可以从 uniform 传入，或者假设在 view 矩阵中提取）
layout(set = 0, binding = 0) uniform U_Camera {
    vec3 viewPos; // 摄像机世界空间位置
} camera;

// Vertex inputs
layout(location = 0) in vec3 inPos;      // from Vertex::pos
layout(location = 1) in vec2 inTexCoord; // from Vertex::texCoord
layout(location = 2) in vec3 inNormal;   // from Vertex::normal
layout(location = 3) in vec4 inTangent;  // Available if needed

// Varyings passed to the fragment shader
layout(location = 0) out vec2 fragTexCoord; // 纹理坐标
layout(location = 1) out vec3 fragNormal;   // 世界空间法线
layout(location = 2) out vec3 fragPos;      // 世界空间位置
layout(location = 3) out vec3 viewPos;      // 摄像机位置
layout(location = 4) out vec4 fragTangent;  // 切线，用于法线贴图

void main() {
    // 计算顶点位置
    vec4 worldPos = renderInfo.data.model * vec4(inPos, 1.0);
    gl_Position = renderInfo.data.proj * renderInfo.data.view * worldPos;

    // 传纹理坐标
    fragTexCoord = inTexCoord;

    // 传世界空间法线
    fragNormal = normalize(mat3(transpose(inverse(renderInfo.data.model))) * inNormal);

    // 传世界空间位置
    fragPos = worldPos.xyz;

    // 传摄像机位置
    viewPos = camera.viewPos;

    // 传切线（用于法线贴图）
    fragTangent = inTangent;
}
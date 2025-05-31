#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// 推送常量，接收纹理索引
layout(push_constant) uniform PushConstants {
    uint textureIndex;
} pushConstants;

// 动态纹理数组，支持任意数量的 baseColor 纹理
layout(set = 0, binding = 1) uniform sampler2D baseColors[];

// 从顶点着色器接收的变量
layout(location = 0) in vec2 fragTexCoord; // 纹理坐标
layout(location = 1) in vec3 fragNormal;   // 世界空间法线

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    // 用推送常量选择纹理
    vec4 textureColor = texture(baseColors[pushConstants.textureIndex], flippedTexCoord);

    // 输出纹理颜色
    outColor = textureColor;
}
#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// 推送常量，接收纹理索引
layout(push_constant) uniform PushConstants {
    uint textureIndex;
} pushConstants;

// 纹理数组
layout(set = 0, binding = 1) uniform sampler2D baseColors[];
layout(set = 0, binding = 2) uniform sampler2D emissiveTextures[];
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessTextures[];
layout(set = 0, binding = 4) uniform sampler2D normalTextures[];

// 从顶点着色器接收的变量
layout(location = 0) in vec2 fragTexCoord; // 纹理坐标
layout(location = 1) in vec3 fragNormal;   // 世界空间法线
layout(location = 2) in vec3 fragPos;      // 世界空间位置
layout(location = 3) in vec3 viewPos;      // 摄像机位置
layout(location = 4) in vec4 fragTangent;  // 切线

// 输出颜色
layout(location = 0) out vec4 outColor;

// 光照参数（优化后的值）
const vec3 lightDir = normalize(vec3(0.25, .80, -0.5)); // 调整光源方向，更自然
const vec3 lightColor = vec3(1.0, 0.95, 0.9);        // 暖色光，增加真实感
const float ambientStrength = 0.62;                    // 增强环境光，避免太暗
const float emissiveStrength = 0.5;                   // 降低发光强度，避免过亮

void main() {
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    uint idx = pushConstants.textureIndex;

    // 读取 baseColor
    vec4 baseColor = texture(baseColors[idx], flippedTexCoord);

    // 读取 emissive
    vec3 emissive = vec3(0.0);
    if (textureSize(emissiveTextures[idx], 0).x > 0) {
        emissive = texture(emissiveTextures[idx], flippedTexCoord).rgb * emissiveStrength;
    }

    // 读取 metallicRoughness
    float metallic = 0.0;
    float roughness = 1.0;
    if (textureSize(metallicRoughnessTextures[idx], 0).x > 0) {
        vec4 mr = texture(metallicRoughnessTextures[idx], flippedTexCoord);
        metallic = mr.b;  // glTF 标准：B 通道是 metallic
        roughness = mr.g; // G 通道是 roughness
    }

    // 读取 normal 并用 TBN 变换
    vec3 normal = normalize(fragNormal);
    if (textureSize(normalTextures[idx], 0).x > 0) {
        vec3 tangent = normalize(fragTangent.xyz);
        vec3 bitangent = normalize(cross(normal, tangent) * fragTangent.w);
        mat3 TBN = mat3(tangent, bitangent, normal);
        vec3 normalMap = texture(normalTextures[idx], flippedTexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0; // 映射到 [-1, 1]
        normal = normalize(TBN * normalMap);
    }

    // 计算光照
    vec3 ambient = ambientStrength * baseColor.rgb;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * baseColor.rgb * lightColor;

    // 镜面反射（优化）
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong 模型，更真实
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0 * (1.0 - roughness));
    vec3 specular = metallic * spec * lightColor * (1.0 - roughness); // 降低镜面强度

    // 最终颜色
    vec3 color = ambient + diffuse + specular + emissive;
    outColor = vec4(color, baseColor.a);
}
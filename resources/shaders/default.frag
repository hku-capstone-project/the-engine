#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform PushConstants {
    uint textureIndex;
} pushConstants;

layout(set = 0, binding = 1) uniform sampler2D baseColors[];
layout(set = 0, binding = 2) uniform sampler2D emissiveTextures[];
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessTextures[];
layout(set = 0, binding = 4) uniform sampler2D normalTextures[];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = normalize(vec3(0.95, 0.95, -0.02));
const vec3 lightColor = vec3(1.0, 0.95, 0.9);
const float ambientStrength = 0.62;
const float emissiveStrength = 0.5;

void main() {
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    uint idx = pushConstants.textureIndex;

    // baseColor
    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
    if (textureSize(baseColors[idx], 0).x > 0) {
        baseColor = texture(baseColors[idx], flippedTexCoord);
    }

    // emissive
    vec3 emissive = vec3(0.0);
    if (textureSize(emissiveTextures[idx], 0).x > 0) {
        emissive = texture(emissiveTextures[idx], flippedTexCoord).rgb * emissiveStrength;
        vec3 obase = baseColor.rgb;
        if (length(obase) < 0.05) {
            baseColor = vec4(1.0);
            emissive *= 0.0;
        }
    }

    // metallicRoughness with occlusion
    float metallic = 0.0;
    float roughness = 0.5;
    float occlusion = 1.0;
    if (textureSize(metallicRoughnessTextures[idx], 0).x > 0) {
        vec4 mr = texture(metallicRoughnessTextures[idx], flippedTexCoord);
        occlusion = mr.r;
        roughness = mr.g;
        metallic = mr.b;
    }

    // normal (improved algorithm)
    vec3 normal = normalize(fragNormal);
    if (textureSize(normalTextures[idx], 0).x > 0) {
        // Ensure tangent space is valid
        vec3 tangent = normalize(fragTangent.xyz);
        vec3 bitangent = normalize(cross(fragNormal, tangent) * fragTangent.w); // Use fragNormal for robustness
        mat3 TBN = mat3(tangent, bitangent, normal);
        // Check if TBN is orthogonal
        if (abs(determinant(TBN)) < 0.1) {
            TBN = mat3(tangent, cross(tangent, normal), normal); // Reconstruct if degenerate
        }
        vec3 normalMap = texture(normalTextures[idx], flippedTexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0; // Standard OpenGL format
        normal = normalize(TBN * normalMap);
    }

    // 光照计算
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Fresnel (Schlick近似)
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, clamp(metallic, 0.0, 1.0));
    float cosTheta = max(dot(viewDir, normal), 0.0);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // 漫反射和镜面反射
    vec3 ambient = ambientStrength * baseColor.rgb * (1.0 - metallic) * occlusion;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * baseColor.rgb * lightColor * (1.0 - metallic);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0 * (1.0 - roughness));
    vec3 specular = fresnel * spec * lightColor;

    // 最终颜色
    vec3 color = ambient + diffuse + specular + emissive;
    outColor = vec4(color, baseColor.a);
}
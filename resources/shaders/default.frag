#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec4 fragTangent;
layout(location = 5) flat in int materialIndex;

layout(location = 0) out vec4 outColor;

const float ambientStrength = 0.62;
const float emissiveStrength = 0.5;

// 定义多个点光源
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

#define NUM_LIGHTS 3
const PointLight lights[NUM_LIGHTS] = PointLight[](
    PointLight(vec3(5.0, 5.0, 5.0), vec3(1.0, 0.95, 0.9), 1.0),
    PointLight(vec3(-5.0, 3.0, -2.0), vec3(0.8, 0.8, 1.0), 0.8),
    PointLight(vec3(0.0, -4.0, 3.0), vec3(1.0, 0.7, 0.7), 0.6)
);

#include "include/sharedVariables.glsl"

layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; } renderInfo;
layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 2) uniform U_MaterialBuffer { S_MaterialInfo materials[64]; } materialBuffer;

void main() {
    // Get material data using the material index with bounds checking
    int safeIndex = clamp(materialIndex, 0, 63); // Ensure index is within bounds [0, 63]
    S_MaterialInfo material = materialBuffer.materials[safeIndex];
    
    vec4 baseColor = vec4(material.color, 1.0);
    float metallic = material.metallic;
    float roughness = material.roughness;
    float occlusion = material.occlusion;
    vec3 emissive = material.emissive * emissiveStrength;
    
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 textureColor = texture(baseColorSampler, flippedTexCoord);

    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPos);
    
    // 环境光
    vec3 ambient = ambientStrength * baseColor.rgb * (1.0 - metallic) * occlusion;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // Fresnel (Schlick近似)
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, clamp(metallic, 0.0, 1.0));
    float cosTheta = max(dot(viewDir, normal), 0.0);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // 多光源计算
    for (int i = 0; i < NUM_LIGHTS; ++i) {
        vec3 lightDir = normalize(lights[i].position - fragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        // 漫反射
        float diff = max(dot(normal, lightDir), 0.0);
        diffuse += diff * baseColor.rgb * lights[i].color * lights[i].intensity * (1.0 - metallic);

        // 镜面反射
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0 * (1.0 - roughness));
        specular += fresnel * spec * lights[i].color * lights[i].intensity;
    }

    // 最终颜色
    vec3 color = ambient + diffuse + specular + emissive;
    outColor = vec4(color, baseColor.a);
}
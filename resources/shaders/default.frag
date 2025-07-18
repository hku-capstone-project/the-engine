#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

const float ambientStrength = 0.1;
const float PI = 3.14159265359;
const float RECIPROCAL_PI = 1.0 / PI;

#include "include/sharedVariables.glsl"

// 纹理绑定
layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; } renderInfo;
layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 3) uniform sampler2D normalSampler;
layout(set = 0, binding = 4) uniform sampler2D metalRoughnessSampler;
layout(set = 0, binding = 5) uniform sampler2D emissiveSampler;
layout(set = 0, binding = 2) uniform U_MaterialInfo { S_MaterialInfo data; } materialInfo;

// PBR BRDF函数
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 修改后的天空光函数，增加albedo参数
vec3 calculateSkyLight(vec3 normal, vec3 viewDir, vec3 F0, float roughness, float metallic, vec3 albedo) {
    // 天空颜色梯度 (从地平线到天顶)
    vec3 horizonColor = vec3(0.5, 0.7, 1.0);
    vec3 zenithColor = vec3(0.1, 0.3, 0.8);
    float horizonBlend = pow(1.0 - abs(normal.y), 2.0);
    vec3 skyColor = mix(zenithColor, horizonColor, horizonBlend);
    
    // 基于物理的环境光照
    vec3 irradiance = skyColor * (0.5 + 0.5 * normal.y);
    
    // 漫反射部分
    vec3 kS = fresnelSchlick(max(dot(normal, viewDir), 0.0), F0);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * irradiance * albedo * RECIPROCAL_PI;
    
    // 粗糙度相关的环境镜面反射 (简化版)
    float smoothness = 1.0 - roughness;
    vec3 reflectVec = reflect(-viewDir, normal);
    vec3 specular = skyColor * pow(max(dot(reflectVec, normal), 0.0), 8.0) * smoothness;
    
    return (diffuse + specular) * ambientStrength;
}

void main() {
    // 材质参数获取
    vec4 baseColor = vec4(materialInfo.data.color, 1.0);
    float metallic = materialInfo.data.metallic;
    float roughness = materialInfo.data.roughness;
    float occlusion = materialInfo.data.occlusion;
    vec3 emissive = materialInfo.data.emissive;

    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);

    // 纹理采样
    if (materialInfo.data.hasBaseColorTex == 1) {
        baseColor = texture(baseColorSampler, flippedTexCoord);
    }

    if (materialInfo.data.hasMetalRoughnessTex == 1) {
        vec3 mr = texture(metalRoughnessSampler, flippedTexCoord).rgb;
        roughness = mr.g;
        metallic = mr.b;
    }

    if (materialInfo.data.hasEmissiveTex == 1) {
        emissive = texture(emissiveSampler, flippedTexCoord).rgb;
    }

    // 法线计算
    vec3 normal = normalize(fragNormal);
    if (materialInfo.data.hasNormalTex == 1) {
        vec3 tangentNormal = texture(normalSampler, flippedTexCoord).xyz * 2.0 - 1.0;
        vec3 T = normalize(fragTangent.xyz);
        vec3 B = normalize(cross(normal, T) * fragTangent.w);
        mat3 TBN = mat3(T, B, normal);
        normal = normalize(TBN * tangentNormal);
    }

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);

    // 修正后的天空光照调用
    vec3 ambient = calculateSkyLight(normal, viewDir, F0, roughness, metallic, baseColor.rgb) * occlusion;

    // 直接光照计算 (太阳)
    vec3 Lo = vec3(0.0);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    vec3 lightColor = vec3(1.0, 0.95, 0.9);
    float lightIntensity = 5.0;
    
    // BRDF计算
    vec3 H = normalize(viewDir + lightDir);
    float NDF = DistributionGGX(normal, H, roughness);        
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);      
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    
    float NdotL = max(dot(normal, lightDir), 0.0);        
    Lo += (kD * baseColor.rgb * RECIPROCAL_PI + specular) * lightColor * lightIntensity * NdotL;

    // 最终颜色合成
    vec3 color = ambient + Lo + emissive;
    
    // 色调映射和伽马校正
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, baseColor.a);
}

#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

const float ambientStrength = 0.62;
const float emissiveStrength = 0.5;

#include "include/sharedVariables.glsl"

layout(set = 0, binding = 0) uniform U_RenderInfo { S_RenderInfo data; } renderInfo;
layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 3) uniform sampler2D normalSampler;
layout(set = 0, binding = 4) uniform sampler2D metalRoughnessSampler;
layout(set = 0, binding = 5) uniform sampler2D emissiveSampler;
layout(set = 0, binding = 2) uniform U_MaterialInfo { S_MaterialInfo data; } materialInfo;

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

const vec3 period = vec3(20.0, 20.0, 20.0);


// Hemisphere light function for sky light simulation
vec3 hemisphere_light(vec3 normal, vec3 sky, vec3 ground, float scaleSky, float scaleGround) {
    float t = dot(normal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;  // Remap dot product to 0-1
    return mix(ground * scaleGround, sky * scaleSky, smoothstep(0.0, 1.0, t)); // 使用smoothstep使过渡更柔和
}

void main() {
    vec4 baseColor = vec4(materialInfo.data.color, 1.0);
    float metallic = materialInfo.data.metallic;
    float roughness = materialInfo.data.roughness;
    float occlusion = materialInfo.data.occlusion;
    vec3 emissive = materialInfo.data.emissive * emissiveStrength;

    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);

    if (materialInfo.data.hasBaseColorTex == 1) {
        baseColor = texture(baseColorSampler, flippedTexCoord);
    }

    if (materialInfo.data.hasMetalRoughnessTex == 1) {
        vec3 mr = texture(metalRoughnessSampler, flippedTexCoord).rgb;
        occlusion = mr.r;
        roughness = mr.g;
        metallic = mr.b;
    } else {
        occlusion = 1.0;
        roughness = 1.0;
        metallic = 0.0;
    }

    if (materialInfo.data.hasEmissiveTex == 1) {
        emissive = texture(emissiveSampler, flippedTexCoord).rgb * emissiveStrength;
    }

    vec3 normal = normalize(fragNormal);
    if (materialInfo.data.hasNormalTex == 1) {
        vec3 tangentNormal = texture(normalSampler, flippedTexCoord).xyz * 2.0 - 1.0;
        vec3 T = normalize(fragTangent.xyz);
        vec3 B = normalize(cross(normal, T) * fragTangent.w);
        mat3 TBN = mat3(T, B, normal);
        normal = normalize(TBN * tangentNormal);
    } 

    vec3 viewDir = normalize(viewPos - fragPos);

    // Sky light parameters (clear screen color simulation)
    vec3 skyColor = vec3(0.3, 0.6, 0.8);  // Deeper blue sky
    vec3 groundColor = vec3(0.3, 0.5, 0.2);  // Greenish ground

    // Hemisphere sky light as ambient + diffuse
    vec3 skyLight = hemisphere_light(normal, skyColor, groundColor, 1.0, 0.5);
    vec3 ambient = ambientStrength * baseColor.rgb * (1.0 - metallic) * occlusion * skyLight;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // Fresnel (Schlick approximation) - keep for specular if needed, but no directional light, so specular minimal
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, clamp(metallic, 0.0, 1.0));
    float cosTheta = max(dot(viewDir, normal), 0.0);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // Simulate a soft directional light from sky for diffuse and specular
    vec3 lightDir = normalize(vec3(0.0, 1.0, 0.5));  // Upward biased for sky
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    diffuse += diff * baseColor.rgb * skyColor * (1.0 - metallic);

    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0 * (1.0 - roughness)); // 降低shininess使specular更柔和
    specular += fresnel * spec * skyColor * 0.5; // 降低specular强度
    
    // 多光源计算（无限网格分布，只计算每个类型最近的一个）
    vec3 half_period = period * 0.5;
    for (int i = 0; i < NUM_LIGHTS; ++i) {
        vec3 vec_to_base = lights[i].position - fragPos;
        vec3 wrapped_vec_to_light = mod(vec_to_base + half_period, period) - half_period;
        float dist = length(wrapped_vec_to_light);
        if (dist < 0.01) continue; // 避免除零
        vec3 lightDir = normalize(wrapped_vec_to_light);  // 修正方向，从frag到light
        float atten = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist); // 添加柔和衰减，避免过亮或黑边
        vec3 halfwayDir = normalize(lightDir + viewDir);

        // 漫反射
        float diff = max(dot(normal, lightDir), 0.0);
        diffuse += diff * baseColor.rgb * lights[i].color * lights[i].intensity * atten * (1.0 - metallic);

        // 镜面反射
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0 * (1.0 - roughness)); // 降低shininess
        specular += fresnel * spec * lights[i].color * lights[i].intensity * atten * 0.5; // 降低强度
    }
    
    // 加入太阳光（directional light）
    vec3 sun_dir = normalize(vec3(0.5, 1.0, 0.3));  // 太阳方向
    vec3 sun_color = vec3(1.0, 0.95, 0.9);
    float sun_intensity = 1.2;
    vec3 sun_light_dir = sun_dir;
    vec3 sun_halfway_dir = normalize(sun_light_dir + viewDir);
    float sun_diff = max(dot(normal, sun_light_dir), 0.0);
    diffuse += sun_diff * baseColor.rgb * sun_color * sun_intensity * (1.0 - metallic);
    float sun_spec = pow(max(dot(normal, sun_halfway_dir), 0.0), 32.0 * (1.0 - roughness)); // 降低shininess
    specular += fresnel * sun_spec * sun_color * sun_intensity * 0.5; // 降低强度
    
    // Final color
    vec3 color = ambient + diffuse + specular + emissive;
    outColor = vec4(color, baseColor.a);
}
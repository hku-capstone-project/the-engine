#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = normalize(vec3(0.35, -0.45, -0.32));
const vec3 lightColor = vec3(1.0, 0.95, 0.9);
const float ambientStrength = 0.62;
const float emissiveStrength = 0.5;

layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;

void main() {

    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
    float metallic = 0.0;
    float roughness = 0.5;
    float occlusion = 1.0;
    vec3 emissive = vec3(0.0);
    
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 textureColor    = texture(baseColorSampler, flippedTexCoord);

    baseColor.rgb = textureColor.rgb;
    vec3 normal = normalize(fragNormal);
    
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

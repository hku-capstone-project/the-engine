#ifndef SHARED_VARIABLES_GLSL
#define SHARED_VARIABLES_GLSL

struct S_RenderInfo {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos; // 添加摄像机位置
    float padding; // 填充对齐
};

struct S_MaterialInfo {
    vec3 color;      // offset 0, size 12
    float metallic;       // offset 12, size 4
    float roughness;      // offset 16, size 4
    float occlusion;      // offset 20, size 4
    vec3 emissive;   // offset 24, size 12
    float padding;        // offset 36, size 4 (填充到 16 字节对齐)
};

struct S_InstanceData {
    mat4 modelMatrix;
    int materialIndex;
    float padding1;
    float padding2;
    float padding3;
};

#endif // SHARED_VARIABLES_GLSL

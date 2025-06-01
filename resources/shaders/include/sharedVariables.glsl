#ifndef SHARED_VARIABLES_GLSL
#define SHARED_VARIABLES_GLSL

// include/sharedVariables.glsl
struct S_RenderInfo {
    mat4 view;
    mat4 proj;
    mat4 model;
    vec3 viewPos; // 添加摄像机位置
    float padding; // 填充对齐
};

#endif // SHARED_VARIABLES_GLSL

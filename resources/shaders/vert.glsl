// vert.glsl
#version 450

// vertex inputs
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

// varyings to the fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    // assume inPos is already in clip‐space (or NDC)
    gl_Position = vec4(inPos, 1.0);

    // pass through
    fragColor = inColor;
    fragUV    = inUV;
}

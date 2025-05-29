// frag.glsl
#version 450

// --- set 0 / binding 1: base‐color sampler ---
// layout(set = 0, binding = 1) uniform sampler2D baseColor;

// --- varyings from the vertex shader ---
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

// --- final output ---
layout(location = 0) out vec4 outColor;

void main() {
    // vec4 tex = texture(baseColor, fragUV);
    // modulate texture by vertex color
    outColor = vec4(fragColor, 1.0);
}

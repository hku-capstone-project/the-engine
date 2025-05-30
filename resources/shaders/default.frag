#version 450

// Texture sampler for the base color
// This corresponds to _images.baseColor in your Renderer.cpp
// layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;

// Varyings received from the vertex shader
layout(location = 0) in vec2 fragTexCoord; // Texture coordinates
layout(location = 1) in vec3 fragNormal;   // World-space normal

// Final output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample the texture using the interpolated texture coordinates
    // vec4 textureColor = texture(baseColorSampler, fragTexCoord);

    // For now, just output the texture color
    // outColor = textureColor;
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // Placeholder color (red)

    // Example: Basic diffuse lighting (optional)
    // vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.5)); // Example light direction
    // float diffuseIntensity = max(dot(fragNormal, lightDirection), 0.0);
    // vec3 diffuseColor = textureColor.rgb * diffuseIntensity;
    // Add some ambient light
    // vec3 ambientColor = textureColor.rgb * 0.2;
    // outColor = vec4(ambientColor + diffuseColor, textureColor.a);

    // If the texture has an alpha channel and blending is enabled (as in your GfxPipeline),
    // this will allow transparency.
}

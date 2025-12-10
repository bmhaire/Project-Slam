#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragUV;

// G-buffer outputs
layout(location = 0) out vec4 outPosition;   // RGB = world position
layout(location = 1) out vec4 outNormal;     // RGB = world normal
layout(location = 2) out vec4 outAlbedo;     // RGB = albedo, A = AO
layout(location = 3) out vec2 outMaterial;   // R = roughness, G = metallic

void main() {
    outPosition = vec4(fragWorldPos, 1.0);
    outNormal = vec4(normalize(fragNormal), 0.0);
    outAlbedo = vec4(fragColor, 1.0);  // AO = 1.0 for now
    outMaterial = vec2(0.5, 0.0);      // Roughness = 0.5, Metallic = 0
}

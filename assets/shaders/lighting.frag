#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// G-buffer samplers
layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMaterial;
layout(binding = 4) uniform sampler2D gDepth;

// Point light structure
struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

// Light uniforms
layout(std140, binding = 6) uniform LightUniforms {
    vec4 cameraPosition;
    vec4 ambientColor;
    uint numLights;
    uint pad[3];
} lightUniforms;

// Light buffer
layout(std430, binding = 5) readonly buffer LightBuffer {
    PointLight lights[];
};

// Cluster data
struct LightCluster {
    uint offset;
    uint count;
};

layout(std430, binding = 7) readonly buffer ClusterBuffer {
    LightCluster clusters[];
};

layout(std430, binding = 8) readonly buffer LightIndexBuffer {
    uint lightIndices[];
};

// Shadow maps (cubemap array)
layout(binding = 9) uniform samplerCubeArrayShadow shadowMaps;

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 invViewProj;
    vec4 cameraPos;
    vec4 screenSize;  // xy = size, z = near, w = far
} push;

// Cluster dimensions
const uint CLUSTER_X = 16;
const uint CLUSTER_Y = 9;
const uint CLUSTER_Z = 24;
const uint MAX_SHADOW_LIGHTS = 8;

// PBR functions
const float PI = 3.14159265359;

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

// Get cluster index from screen position and depth
uint getClusterIndex(vec2 screenPos, float depth) {
    uint x = uint(screenPos.x / push.screenSize.x * float(CLUSTER_X));
    uint y = uint(screenPos.y / push.screenSize.y * float(CLUSTER_Y));

    // Exponential depth slicing
    float near = push.screenSize.z;
    float far = push.screenSize.w;
    float logDepth = log(depth / near) / log(far / near);
    uint z = uint(logDepth * float(CLUSTER_Z));

    x = min(x, CLUSTER_X - 1);
    y = min(y, CLUSTER_Y - 1);
    z = min(z, CLUSTER_Z - 1);

    return x + y * CLUSTER_X + z * CLUSTER_X * CLUSTER_Y;
}

// Calculate shadow for point light
float calculateShadow(uint lightIndex, vec3 fragPos, vec3 lightPos, float lightRadius) {
    if (lightIndex >= MAX_SHADOW_LIGHTS) {
        return 1.0;  // No shadow for lights beyond shadow limit
    }

    vec3 lightToFrag = fragPos - lightPos;
    float currentDepth = length(lightToFrag) / lightRadius;

    // Sample shadow cubemap
    float shadow = texture(shadowMaps, vec4(lightToFrag, float(lightIndex)), currentDepth - 0.005);

    return shadow;
}

void main() {
    // Sample G-buffer
    vec3 worldPos = texture(gPosition, fragUV).rgb;
    vec3 normal = normalize(texture(gNormal, fragUV).rgb);
    vec4 albedoAO = texture(gAlbedo, fragUV);
    vec2 material = texture(gMaterial, fragUV).rg;
    float depth = texture(gDepth, fragUV).r;

    // Early out for background
    if (depth >= 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 albedo = albedoAO.rgb;
    float ao = albedoAO.a;
    float roughness = material.r;
    float metallic = material.g;

    // Calculate view direction
    vec3 V = normalize(push.cameraPos.xyz - worldPos);
    vec3 N = normal;

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Ambient lighting
    vec3 ambient = lightUniforms.ambientColor.rgb * albedo * ao;
    vec3 Lo = vec3(0.0);

    // Get cluster for this fragment
    vec2 screenPos = gl_FragCoord.xy;
    float linearDepth = length(worldPos - push.cameraPos.xyz);
    uint clusterIndex = getClusterIndex(screenPos, linearDepth);

    LightCluster cluster = clusters[clusterIndex];

    // Process lights in cluster
    for (uint i = 0; i < cluster.count; i++) {
        uint lightIndex = lightIndices[cluster.offset + i];
        PointLight light = lights[lightIndex];

        vec3 L = normalize(light.position - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(light.position - worldPos);

        // Attenuation
        float attenuation = 1.0 / (distance * distance);
        float falloff = clamp(1.0 - pow(distance / light.radius, 4.0), 0.0, 1.0);
        falloff = falloff * falloff;
        attenuation *= falloff;

        vec3 radiance = light.color * light.intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        // Shadow
        float shadow = calculateShadow(lightIndex, worldPos, light.position, light.radius);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow;
    }

    vec3 color = ambient + Lo;

    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}

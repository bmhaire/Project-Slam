#version 450

// Inputs from vertex shader
layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in vec3 frag_world_pos;

// Output
layout(location = 0) out vec4 out_color;

void main() {
    // Simple directional lighting
    vec3 light_dir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 normal = normalize(frag_normal);

    // Ambient
    vec3 ambient = 0.2 * frag_color;

    // Diffuse
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * frag_color;

    // Simple specular (Blinn-Phong)
    vec3 view_dir = normalize(-frag_world_pos); // Approximate view direction
    vec3 half_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
    vec3 specular = 0.3 * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    out_color = vec4(result, 1.0);
}

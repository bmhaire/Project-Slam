#version 450

// Vertex inputs
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_uv;

// Push constants - MVP matrices
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pc;

// Outputs to fragment shader
layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out vec3 frag_world_pos;

void main() {
    // Transform to world space
    vec4 world_pos = pc.model * vec4(in_position, 1.0);
    frag_world_pos = world_pos.xyz;

    // Transform to clip space
    gl_Position = pc.projection * pc.view * world_pos;

    // Pass through other attributes
    frag_color = in_color;

    // Transform normal to world space (assuming uniform scale)
    mat3 normal_matrix = mat3(pc.model);
    frag_normal = normalize(normal_matrix * in_normal);

    frag_uv = in_uv;
}

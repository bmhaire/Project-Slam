#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    vec4 lightPos;  // xyz = position, w = far plane
} push;

void main() {
    gl_Position = push.lightSpaceMatrix * vec4(inPosition, 1.0);
}

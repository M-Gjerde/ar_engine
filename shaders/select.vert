#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec4 frag_color;

layout (push_constant) uniform PushConstants {
    vec4 color;
} pushConstants;

void main() {
    gl_Position = vec4(position, 1.0);
    frag_color = pushConstants.color;
}
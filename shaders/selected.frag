#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 0) out vec4 f_color;

layout (push_constant) uniform PushConstants {
    int isSelected;
} pushConstants;

void main() {
    if (pushConstants.isSelected == 0) {
        f_color = vec4(frag_color, 1.0);
    } else {
        f_color = vec4(1.0, 1.0, 1.0, 1.0);
    }
}

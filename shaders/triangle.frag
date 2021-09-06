#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 normals;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec4 outColor;

void main() {

  outColor = vec4(0.25, 0.25, 0.25, 1);
}
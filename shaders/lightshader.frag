#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 1, set = 1) uniform Colors {
    vec3 objectColor;
    vec3 lightColor;
} colors;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(colors.lightColor * colors.objectColor, 1.0);
    //FragColor = vec4(0,0,0,1);
}
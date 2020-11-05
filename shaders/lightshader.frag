#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;

layout(binding = 1, set = 1) uniform Colors {
    vec4 objectColor;
    vec4 lightColor;
    vec4 lightPos;
} colors;

layout(location = 0) out vec4 FragColor;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * colors.lightColor.xyz;
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(colors.lightPos.xyz - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * colors.lightColor.xyz;

    vec3 result = (ambient + diffuse) * colors.objectColor.xyz;
    FragColor = vec4(result, 1.0);

}
#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 FragPos;


layout(set = 0, binding = 1) uniform INFO {
    vec4 lightColor;
    vec4 objectColor;
    vec4 lightPos;
    vec4 viewPos;

} info;

layout(set = 0, binding = 2) uniform SELECT {
    float map;
} select;

layout (set = 0, binding = 3) uniform sampler2D samplerColorMap;
layout (set = 0, binding = 4) uniform sampler2D samplerTextureMap;

layout (location = 0) out vec4 outFragColor;

void main()
{

    if (select.map == 0){
    }

    if (select.map == 1){
    }
    if (select.map == 2){

    }

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * info.lightColor.rgb;

    // diffuse
    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(info.lightPos.xyz - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * info.lightColor.rgb;

    // specular
    float specularStrength = 0.7;
    vec3 viewDir = normalize(info.viewPos.xyz - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
    vec3 specular = specularStrength * spec * info.lightColor.rgb;



    //vec3 result = (ambient + diffuse + specular) * color;
    vec3 result = texture(samplerColorMap, inUV).rgb;

    outFragColor = vec4(result, 1.0);

    //color = vec4(0.3, 0.3, 0.3, 1.0);
    //outFragColor = color;

}
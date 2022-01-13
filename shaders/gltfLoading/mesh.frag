#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (set = 0, binding = 2) uniform sampler2D samplerColorMap;
layout (set = 0, binding = 3) uniform sampler2D samplerTextureMap;

layout(set = 0, binding = 1) uniform SELECT {
    float map;
} select;


layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 color = vec3(0.7, 0.7, 0.7);
    vec3 normals = normalize(inNormal);

    if (select.map == 0){
        color = vec3(0.7, 0.7, 0.7);
    }

    if (select.map == 1){
        color = texture(samplerColorMap, inUV).rgb;
    }
    if (select.map == 2){
        color = texture(samplerColorMap, inUV).rgb;
        vec4 N = texture(samplerTextureMap, inUV);
        //N = normalize(N * 2.0 - 1.0);

    }

    vec3 ambient = color * 0.1;
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = reflect(-L, normals);
    vec3 diffuse = max(dot(normals, L), 0.15) * color;
    vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
    outFragColor = vec4(ambient + diffuse * color.rgb + specular, 1.0);

    //outFragColor = vec4(normals, 1.0f);

    //color = vec4(0.3, 0.3, 0.3, 1.0);
    //outFragColor = color;

}
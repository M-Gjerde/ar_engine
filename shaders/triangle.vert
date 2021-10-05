#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inJoint0;
layout (location = 5) in vec4 inWeight0;

layout (binding = 0) uniform UBO
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;

out gl_PerVertex
{
    vec4 gl_Position;
};


void main()
{
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);

	fragTexCoord = inUV;
	fragPos = vec3(ubo.modelMatrix * vec4(inPos, 1.0));
	outNormal = mat3(transpose(inverse(ubo.modelMatrix))) * inNormal;

}
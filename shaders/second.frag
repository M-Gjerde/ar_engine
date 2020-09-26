#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColour; // Colour output from subpass 1
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput inputDepth;  // Depth output from subpass 1

layout(location = 0) out vec4 colour;

void main()
{
        colour = subpassLoad(inputColour).rgba;
}
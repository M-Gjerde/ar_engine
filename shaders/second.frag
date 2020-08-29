#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColour; // Colour output from subpass 1
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput inputDepth;  // Depth output from subpass 1

layout(location = 0) out vec4 colour;

void main()
{
    int xHalf = 1200/2;
    if(gl_FragCoord.x > xHalf)
    {
        float lowerBound = 0.98;
        float upperBound = 1;

        float depth = subpassLoad(inputDepth).r;
        float depthColourScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));
        //colour = vec4(subpassLoad(inputColour).rgb * depthColourScaled, 1.0f);
        //colour = subpassLoad(inputDepth).rgba;
        colour = subpassLoad(inputColour).rgba;

    }
    else
    {
        colour = subpassLoad(inputColour).rgba;
    }
}
//
// Created by magnus on 9/20/20.
//

#include <array>
#include <iostream>
#include "GraphicsPipeline.h"
#include "../Mesh/Object.h"
#include "../Mesh/Mesh.hpp"

GraphicsPipeline::GraphicsPipeline() = default;

GraphicsPipeline::~GraphicsPipeline() = default;


VkShaderModule GraphicsPipeline::createShaderModule(Utils::MainDevice mainDevice, const std::vector<char> &code) {
    //  Shader module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();                                          // Size of code
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());         // pointer to code -> reinterpret_cast is to convert between pointer cast

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");

    return shaderModule;
}

void GraphicsPipeline::createGraphicsPipeline(Utils::MainDevice mainDevice, VkExtent2D swapchainExtent,
                                              Utils::Pipelines *pipelineObject, Utils::Pipelines *secondPipeline) {
    auto vertexShaderCode = Utils::readFile("../shaders/vert.spv");
    auto fragmentShaderCode = Utils::readFile("../shaders/frag.spv");

    //  Build shader modules to link to graphics pipeline
    VkShaderModule vertexShaderModule = createShaderModule(mainDevice, vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(mainDevice, fragmentShaderCode);

    //  -- SHADER STAGE CREATION INFORMATION --
    //  Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;                  //Shader stage name
    vertexShaderCreateInfo.module = vertexShaderModule;                         //Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                                      //Entry point in to shader

    //  Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;                  //Shader stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;                         //Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                                      //Entry point in to shader

    //  Put shader stage creation info in to array
    //  Graphics pipeline create info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                             // Can bind multiple streams of data, this defined which one
    bindingDescription.stride = sizeof(Vertex);                 // Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Draw one object at a time (VERTEX), or draw multiple object simultaneously (INSTANCING)
    // Only valid for the same object
    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    // Position Attribute
    attributeDescriptions[0].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[0].location = 0;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[0].offset = offsetof(Vertex,
                                               pos);                // We can have multiple attributes in the same data which is separated by nth index

    // Color Attribute
    attributeDescriptions[1].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[1].location = 1;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[1].offset = offsetof(Vertex,
                                               col);                // We can have multiple attributes in the same data which is separated by nth index


// Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);


    //  -- VERTEX INPUT --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;                                     //List of vertex Binding descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();                          //List of vertex attribute descriptions (data format and where to bind to or from)

    //  -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;       //Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;                    //Allow overriding of "strip" topology to start new primitives

    //  -- VIEWPORT & SCISSOR --
    //  Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;                                  //x start coordinate
    viewport.y = 0.0f;                                  //y start coordinate
    viewport.width = (float) swapchainExtent.width;     //viewport width
    viewport.height = (float) swapchainExtent.height;   //viewport height
    viewport.minDepth = 0.0f;                           //min framebuffer depth
    viewport.maxDepth = 1.0f;                           //max framebuffer depth

    //  Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = {0, 0};                      //Offset to use region from
    scissor.extent = swapchainExtent;                   //Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // -- DYNAMIC STATES --
    //  Dynamic states to enable
    /*
    std::vector<VkDynamicState> dynamicStateEnable;
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_VIEWPORT);        //Dynamic viewport : can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport)
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_SCISSOR);         //Dynamic scissor  : can resize in command buffer with vkCmdsetScissor(commandbuffer, 0, 1, &scissor)
    //Make sure to destroy swapchain and recreate after you've resized, which is also the same with the depth buffer.
    //  Dynamic state creation info
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnable.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStateEnable.data();
*/

    //  -- RASTERIZER --
    //  Converts primitives into fragments
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;                       //Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;                //Stop the pipeline process here, which we wont
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;                //How we want our polygons to be filled in color between vertices.
    rasterizerCreateInfo.lineWidth = 1.0f;                                  //How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;                  //Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;               //Winding to dertermine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                        //Wether to add depth bias to fragments (Good for stopping "shadow acne")

    //  -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;                       //Enable multisample shading or not
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;         //Number of samples to user per fragment

    //  -- BLENDING --
    //  Blending decides how to blend a new color being written to a fragment, with the old value

    //  Blend attachment state (how blending is handled)
    VkPipelineColorBlendAttachmentState colorStateAttachment = {};
    colorStateAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_A_BIT;           //Colors to apply blending to
    colorStateAttachment.blendEnable = VK_TRUE;                               //Enable blending
    //  Blending uses equation: (srcColorBlendFactor * new color) colorblendOp (dstColorBlendfactor * old color)
    colorStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorStateAttachment.colorBlendOp = VK_BLEND_OP_ADD;                      //Which mathematical operation we use to calculate color blend
    //  Replace the old alpha channel with the new one
    colorStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorStateAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //  Summarised: (1 * newAlpha) + (0 * oldAlpha) = new alpha
    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;               //Alternative to calculations is to use logical operations
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorStateAttachment;

    //  -- PIPELINE LAYOUT --
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {descriptorSetLayout, samplerSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());         // Number of descriptor set layouts
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();                                   // Descriptor set layout to bind
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, &vkAllocationCallbacks,
                                             &pipelineObject->pipelineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline");

    //  -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;                    // Enable checking depth to determine fragment write (Wether to replace pixel)
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;                   // Can we replace our value when we place a pixel
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;          // If the new value is less then overwrite
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;             // Depth bounds test: Does the depth value exist between two bounds=
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;                 // Enable stencil test

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;                                   //Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                           //List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;       //All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.layout = pipelineObject->pipelineLayout;                         //Pipeline layout pipeline should use
    pipelineCreateInfo.renderPass = pipelineObject->renderPass;                         //Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                     //Subpass of render pass to use with pipeline

    //  Pipeline derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;             //If we want to create a pipeline from an old pipeline
    pipelineCreateInfo.basePipelineIndex = -1;                          //Or index we want to derive pipeline from

    //  Create graphics pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, &vkAllocationCallbacks,
                                       &pipelineObject->pipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
    //  Destroy modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);

    // CREATE SECOND PASS PIPELINE
    // Second pass shaders
    auto secondVertexShaderCode = Utils::readFile("../shaders/second_vert.spv");
    auto secondFragmentShaderCode = Utils::readFile("../shaders/second_frag.spv");

    // Build shaders
    VkShaderModule secondVertexShaderModule = createShaderModule(mainDevice, secondVertexShaderCode);
    VkShaderModule secondFragmentShaderModule = createShaderModule(mainDevice, secondFragmentShaderCode);

    // Set new shaders
    vertexShaderCreateInfo.module = secondVertexShaderModule;
    fragmentShaderCreateInfo.module = secondFragmentShaderModule;

    VkPipelineShaderStageCreateInfo secondShaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};


    // No vertex data for second pass
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Dont want to write to depth buffer
    depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

    // Create new pipeline layout
    VkPipelineLayoutCreateInfo secondPipelineCreateInfo = {};
    secondPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    secondPipelineCreateInfo.setLayoutCount = 1;
    secondPipelineCreateInfo.pSetLayouts = &inputSetLayout;
    secondPipelineCreateInfo.pushConstantRangeCount = 0;
    secondPipelineCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(mainDevice.logicalDevice, &secondPipelineCreateInfo, &vkAllocationCallbacks,
                                    &secondPipeline->pipelineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create second pipeline");

    pipelineCreateInfo.pStages = secondShaderStages;    // Update second shader stage list
    pipelineCreateInfo.layout = secondPipeline->pipelineLayout;   // Change pipeline layout for input attachment descriptor sets
    pipelineCreateInfo.subpass = 1;                     // Use second subpass

    //  Create graphics pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, &vkAllocationCallbacks,
                                       &secondPipeline->pipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create second pipeline");

    //  Destroy second shader modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, secondVertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, secondFragmentShaderModule, nullptr);


}

void GraphicsPipeline::createDescriptorSetLayout(Utils::MainDevice mainDevice) {

    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // VP binding info Create descriptor layout bindings
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
    vpLayoutBinding.descriptorCount = 1;                                   // Number of descriptors for binding
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;               // Shader stage to bind to
    vpLayoutBinding.pImmutableSamplers = nullptr;                          // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed


    // Model binding info
    VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {vpLayoutBinding};
    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                               // Array of binding infos

    // Create descriptor set layout
    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr,
                                                  &descriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor set layout
    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr,
                                         &samplerSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor Set Layout");

    // CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
    // Color input binding
    VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
    colorInputLayoutBinding.binding = 0;
    colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding.descriptorCount = 1;
    colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth input binding
    VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
    depthInputLayoutBinding.binding = 1;
    depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputLayoutBinding.descriptorCount = 1;
    depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Array of input attachment bindings
    std::vector<VkDescriptorSetLayoutBinding> inputBindings = {colorInputLayoutBinding, depthInputLayoutBinding};

    // Create a Descriptor Set Layout for input attachments
    VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
    inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
    inputLayoutCreateInfo.pBindings = inputBindings.data();

    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &inputLayoutCreateInfo, nullptr, &inputSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor Set Layout");

}

void GraphicsPipeline::createPushConstantRange() {
    // Define push constant values
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;      // Shader stage push constant will go to
    pushConstantRange.offset = 0;                                   // Offset into given data to pass to push constat
    pushConstantRange.size = sizeof(Model);                         // Size of data being passed

}

void GraphicsPipeline::createRenderPass(Utils::MainDevice mainDevice, VkFormat swapChainImageFormat,
                                        Utils::Pipelines *renderPass) {

    // Array of our subpasses
    std::array<VkSubpassDescription, 2> subpasses{};

    // ATTACHMENTS
    // SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

    // Color Attachment (Input)
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = Utils::chooseSupportedFormat(mainDevice, {VK_FORMAT_R8G8B8A8_UNORM},
                                                          VK_IMAGE_TILING_OPTIMAL,
                                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //Depth Attachment (Input)
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = Utils::chooseSupportedFormat(mainDevice,
                                                          {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
                                                           VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                                                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Color attachment (Input) Reference
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 1;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment (Input) Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 2;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set up subpass 1
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorAttachmentReference;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;


    // SUBPASS 2 ATTACHMENT + REFERENCES

    // SwapChain Color Attachment of render pass
    VkAttachmentDescription swapChainColorAttachment = {};
    swapChainColorAttachment.format = swapChainImageFormat;                      //Format to use for attachment
    swapChainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    //Number of samples to write for multisampling
    swapChainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               //Start with a blank state when we draw our image, describes what to do with attachment before rendering
    swapChainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             //Describes what to do with attachment after rendering
    swapChainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    //Describes what to do with stencil before rendering
    swapChainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  //Describes what to do with stencil before rendering

    //  outdated after subpass: Framebuffer data will be stored as an image, but images can be given different data layouts
    //  to give optimal use for certain operations
    swapChainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          //Image data layout before render pass starts
    swapChainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      //Image data layout after render pass starts (to change to)

    //  Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference swapChainColorAttachmentReference = {};
    swapChainColorAttachmentReference.attachment = 0;
    swapChainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;     //Will be converted to this type after initalLayout in colorAttachment

    // References to attachments that subpass will take input from
    std::array<VkAttachmentReference, 2> inputReferences{};
    inputReferences[0].attachment = 1;
    inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputReferences[1].attachment = 2;
    inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Set up subpass 2
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &swapChainColorAttachmentReference;
    subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
    subpasses[1].pInputAttachments = inputReferences.data();


    // SUBPASS DEPENDENCIES

    //  Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 3> subpassDependencies{};

    //  Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    //  Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        //Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     //Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               //Stage access mask (memory access)

    //  But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;
    // Subpass 1 layout (Color/depth) to Subpass 2 Layout (shader read)
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstSubpass = 1;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    //  Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    //  Transition must happen after...
    subpassDependencies[2].srcSubpass = 0;
    subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[2].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //  But must happen before...
    subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[2].dependencyFlags = 0;

    // Array with attachments
    std::array<VkAttachmentDescription, 3> renderPassAttachments = {swapChainColorAttachment, colorAttachment,
                                                                    depthAttachment};

    //  Create info for Render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, &vkAllocationCallbacks, &renderPass->renderPass);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Render pass");

}


void GraphicsPipeline::getPushConstantRange(VkPushConstantRange *pPushConstantRange) {
    createPushConstantRange();
    *pPushConstantRange = pushConstantRange;

}

void GraphicsPipeline::getDescriptorSetLayout(Utils::MainDevice mainDevice, VkDescriptorSetLayout *pDescriptorSetLayout,
                                              VkDescriptorSetLayout *pDescriptorSetLayout1,
                                              VkDescriptorSetLayout *pDescriptorSetLayout2) {
    createDescriptorSetLayout(mainDevice);
    *pDescriptorSetLayout = descriptorSetLayout;
    *pDescriptorSetLayout1 = samplerSetLayout;
    *pDescriptorSetLayout2 = inputSetLayout;

}


void GraphicsPipeline::createBoxPipeline(Utils::MainDevice mainDevice, VkExtent2D swapchainExtent,
                                         Utils::Pipelines *pipelineToBind) {
    auto vertexShaderCode = Utils::readFile("../shaders/boxShaderVert.spv");
    auto fragmentShaderCode = Utils::readFile("../shaders/boxShaderFrag.spv");

    //  Build shader modules to link to graphics pipeline
    VkShaderModule vertexShaderModule = createShaderModule(mainDevice, vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(mainDevice, fragmentShaderCode);

    // --- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    //  Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = fragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    //  Put shader stage creation info in to array
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Box);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    // Position Attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Box, pos);
    // Color Attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Box, col);
    // -- VERTEX INPUT ---
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    //  -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;                    // Enable checking depth to determine fragment write (Wether to replace pixel)
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;                   // Can we replace our value when we place a pixel
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;          // If the new value is less then overwrite
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;             // Depth bounds test: Does the depth value exist between two bounds=
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;                 // Enable stencil test


    //  -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //  -- VIEWPORT & SCISSOR --
    //  Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainExtent.width;
    viewport.height = (float) swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //  Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    //  -- RASTERIZER --
    //  Converts primitives into fragments
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;                       //Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;                //Stop the pipeline process here, which we wont
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;                //How we want our polygons to be filled in color between vertices.
    rasterizerCreateInfo.lineWidth = 1.0f;                                  //How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;                  //Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;               //Winding to dertermine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                        //Wether to add depth bias to fragments (Good for stopping "shadow acne")

    //  -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;                       //Enable multisample shading or not
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;         //Number of samples to user per fragment

    //  -- BLENDING --
    //  Blending decides how to blend a new color being written to a fragment, with the old value

    //  Blend attachment state (how blending is handled)
    VkPipelineColorBlendAttachmentState colorStateAttachment = {};
    colorStateAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_A_BIT;           //Colors to apply blending to
    colorStateAttachment.blendEnable = VK_TRUE;                               //Enable blending
    //  Blending uses equation: (srcColorBlendFactor * new color) colorblendOp (dstColorBlendfactor * old color)
    colorStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorStateAttachment.colorBlendOp = VK_BLEND_OP_ADD;                      //Which mathematical operation we use to calculate color blend
    //  Replace the old alpha channel with the new one
    colorStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorStateAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //  Summarised: (1 * newAlpha) + (0 * oldAlpha) = new alpha
    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;               //Alternative to calculations is to use logical operations
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorStateAttachment;


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;      // Number of descriptor set layouts
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;                             // Descriptor set layout to bind
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, &vkAllocationCallbacks,
                                             &pipelineToBind->pipelineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline");


    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;                                   //Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                           //List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;       //All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.layout = pipelineToBind->pipelineLayout;                         //Pipeline layout pipeline should use
    pipelineCreateInfo.renderPass = pipelineToBind->renderPass;                         //Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                     //Subpass of render pass to use with pipeline

    //  Pipeline derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;             //If we want to create a pipeline from an old pipeline
    pipelineCreateInfo.basePipelineIndex = -1;                          //Or index we want to derive pipeline from

    //  Create graphics pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, &vkAllocationCallbacks,
                                       &pipelineToBind->pipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
    //  Destroy modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);

}

void GraphicsPipeline::createAnotherRenderPass(Utils::MainDevice mainDevice, VkFormat swapchainImageFormat,
                                               Utils::Pipelines *pipes) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(mainDevice.logicalDevice, &renderPassInfo, &vkAllocationCallbacks, &pipes->renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

}

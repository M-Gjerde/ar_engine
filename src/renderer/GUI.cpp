//
// Created by magnus on 8/2/21.
//

#include <algorithm>
#include <utility>
#include "GUI.h"

GUI::GUI(ArEngine mArEngine) {
    device = mArEngine.mainDevice.device;
    arEngine = mArEngine;
    images = new Images(mArEngine.mainDevice, mArEngine.swapchainExtent);
    descriptors = new Descriptors(mArEngine);
    pipeline = new Pipeline();
    ImGui::CreateContext();

    // Prepare buffers
    vertexBuffer = new Buffer(arEngine.mainDevice);
    indexBuffer = new Buffer(arEngine.mainDevice);

    // CmdBuffers for GUI
    /*
    commandBuffers = new CommandBuffers(arEngine);
    commandBuffers->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers->allocateCommandBuffers(3, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
*/

}

GUI::~GUI() {
    ImGui::DestroyContext();

    vertexBuffer->destroy();
    indexBuffer->destroy();

    delete vertexBuffer;
    delete indexBuffer;
    delete images;
    delete descriptors;
    delete pipeline;
    //delete commandBuffers;
}

void GUI::init(uint32_t width, uint32_t height) {
    // Color scheme
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // Dimensions
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) width, (float) height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

}

void GUI::initResources(VkRenderPass renderPass) {
    ImGuiIO &io = ImGui::GetIO();

    // Create font texture
    unsigned char *fontData;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    // --- Create Target image for copy
    images->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, &memReqs);
    // --- Image view
    images->createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &imageView);

    // Staging buffers for font data upload
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBuffer.bufferSize = uploadSize;
    images->createBuffer(&stagingBuffer);

    uint8_t *data;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, stagingBuffer.bufferSize, 0,
                (void **) &data);
    memcpy(data, fontData, uploadSize);
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    // Copy buffer data to font image
    // Prepare for transfer
    // Transition image to transfer destination
    images->transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arEngine.commandPool, arEngine.graphicsQueue);

    // Copy
    // Copy buffer to image
    images->copyBufferToImage(stagingBuffer.buffer, image, texWidth, texHeight, arEngine.commandPool,
                              arEngine.graphicsQueue);
    // DEstroy staging buffer now
    vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);
    // Prepare for shader read
    // Transition image to shader read
    images->transitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arEngine.commandPool,
                                  arEngine.graphicsQueue);
    // --- Font texture Sampler

    images->createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, device, &sampler);

    // --- Descriptor pool
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorPoolCount = 1;
    descriptorInfo.descriptorCount = 1;
    descriptorInfo.descriptorSetLayoutCount = 1;
    descriptorInfo.descriptorSetCount = 1;
    std::vector<uint32_t> descriptorCounts;
    descriptorCounts.push_back(1);
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::vector<uint32_t> bindings;
    bindings.push_back(0);
    descriptorInfo.pBindings = bindings.data();
    // types
    std::vector<VkDescriptorType> types(1);
    types[0] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorInfo.pDescriptorType = types.data();
    // stages
    std::array<VkShaderStageFlags, 1> stageFlags = {VK_SHADER_STAGE_FRAGMENT_BIT};
    descriptorInfo.stageFlags = stageFlags.data();

    descriptorInfo.sampler = sampler;
    descriptorInfo.view = imageView;

    descriptors->createDescriptors(descriptorInfo, &descriptor);

    // --- Descriptor Set Layout

    // --- DescriptorSet

    // Create PIPELINE
    /*
    ArShadersPath shadersPath{};
    shadersPath.vertexShader = "../shaders/textoverlay/ui.vert";
    shadersPath.fragmentShader = "../shaders/textoverlay/ui.frag";

    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;
    pipeline->textRenderPipeline(renderPass, descriptor, shadersPath, &arPipeline, nullptr);
*/
    ArShadersPath shadersPath{};
    shadersPath.vertexShader = "../shaders/textoverlay/ui.vert";
    shadersPath.fragmentShader = "../shaders/textoverlay/ui.frag";
    auto vertShaderCode = readFile(shadersPath.vertexShader + ".spv");
    auto fragShaderCode = readFile(shadersPath.fragmentShader + ".spv");

    VkShaderModule vertShaderModule = Pipeline::createShaderModule(arEngine.mainDevice.device, vertShaderCode);
    VkShaderModule fragShaderModule = Pipeline::createShaderModule(arEngine.mainDevice.device, fragShaderCode);

    //SHADER SETUP
    // Vertex shader info
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // Fragment shader info
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // VERTEX INPUT DESCRIPTION BASED ON IMGUI vertex DEFINITION
    VkVertexInputBindingDescription vertexInputBindingDescription;
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(ImDrawVert);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributeDescription{};
    vertexInputAttributeDescription[0].binding = 0;
    vertexInputAttributeDescription[0].location = 0;
    vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[0].offset = offsetof(ImDrawVert, pos);

    vertexInputAttributeDescription[1].binding = 0;
    vertexInputAttributeDescription[1].location = 1;
    vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[1].offset = offsetof(ImDrawVert, uv);

    vertexInputAttributeDescription[2].binding = 0;
    vertexInputAttributeDescription[2].location = 2;
    vertexInputAttributeDescription[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    vertexInputAttributeDescription[2].offset = offsetof(ImDrawVert, col);

    // Vertex input into pipeline
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescription.data(); // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = Pipeline::inputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    // VIEWPORT SETUP
    VkPipelineViewportStateCreateInfo viewportState = Pipeline::viewportStateCreateInfo(1, 1, 0);
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) arEngine.swapchainExtent.width;
    viewport.height = (float) arEngine.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewportState.pViewports = &viewport;


    // RASTERIZER
    VkPipelineRasterizationStateCreateInfo rasterizer = Pipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL,
                                                                                               VK_CULL_MODE_NONE,
                                                                                               VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                                               0);

    // MULTISAMPLING || ANTIALISING
    VkPipelineMultisampleStateCreateInfo multisampling = Pipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

    // DEPTH STENCIL
    VkPipelineDepthStencilStateCreateInfo depthStencil = Pipeline::depthStencilStateCreateInfo(VK_FALSE, VK_FALSE,
                                                                                               VK_COMPARE_OP_LESS_OR_EQUAL);


    // COLOR BLENDING
    // Enable blending, using alpha from red channel of the font texture (see text.frag)
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = Pipeline::colorBlendStateCreateInfo(1, &blendAttachmentState);

    // Pipeline Layout
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.size = sizeof(pushConstBlock);
    pushConstantRange.offset = 0;

    // CREATE PIPELINE LAYOUT
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptor.descriptorSetLayoutCount;
    pipelineLayoutInfo.pSetLayouts = descriptor.pDescriptorSetLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


    if (vkCreatePipelineLayout(arEngine.mainDevice.device, &pipelineLayoutInfo, nullptr, &arPipeline.pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // DYNAMIC STATES
    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = Pipeline::dynamicStateCreateInfo(dynamicStateEnables);

    VkGraphicsPipelineCreateInfo pipelineInfo = Pipeline::createInfo(arPipeline.pipelineLayout, renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    if (vkCreateGraphicsPipelines(arEngine.mainDevice.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &arPipeline.pipeline) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }


    vkDestroyShaderModule(arEngine.mainDevice.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(arEngine.mainDevice.device, vertShaderModule, nullptr);

}

// Starts a new imGui frame and sets up windows and ui elements
void GUI::newFrame(bool updateFrameGraph) {

    ImGui::NewFrame();
    // Init imGui windows and elements
    ImGui::SetWindowSize("Debug Information", ImVec2(200, 400));
    ImVec4 clear_color = ImColor(114, 144, 154);
    static float f = 0.0f;
    ImGui::TextUnformatted("Vulkan Engine");
    ImGui::TextUnformatted("DeviceName");

    ImGui::PlotLines("Framerate", &uiSettings.frameTimes[0], 1000, 0, "FPS", 0,
                    1000, ImVec2(0, 80));
    ImGui::Text("Avg framerate %.3f", uiSettings.average);
    ImGui::Text("FPS cap:  %.3f", uiSettings.frameLimiter);

    ImGui::Text("Camera");
    float pos[] = {3, 3, 3};
    ImGui::InputFloat3("position", pos, "2");
    ImGui::InputFloat3("rotation", pos, "2");

    // Input Available settings
    // Reload Shader
    // Reload Mesh with different settings
    for (auto & setting : settings) {
        if (setting.active){
            if (setting.type == "slider"){
                ImGui::SliderFloat(setting.name.c_str(), &setting.val, setting.minRange, setting.maxRange);

            }
        }

    }


    // Re-run Mesh generator

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &uiSettings.displayModels);
    ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
    ImGui::Checkbox("Display background", &uiSettings.displayBackground);
    ImGui::Checkbox("Animate light", &uiSettings.animateLight);
    ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Button("Button Label");
    ImGui::End();

    // Render to generate draw buffers
    ImGui::Render();
}


void GUI::cleanUp() {
    vkDestroyImage(device, image, nullptr);
    vkDestroyImageView(device, imageView, nullptr);
    vkDestroyPipeline(device, arPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, arPipeline.pipelineLayout, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);
    descriptors->cleanUp(descriptor);
    vkDestroySampler(device, sampler, nullptr);

}

void GUI::updateBuffers() {

    ImDrawData *imDrawData = ImGui::GetDrawData();

    // Note: Alignment is done inside buffer creation
    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return;
    }

    // Update buffers only if vertex or index count has been changed compared to current buffer size

    // Vertex buffer
    if ((vertexBuffer->buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
        // Destroy buffers
        vertexBuffer->unmap();
        vertexBuffer->destroy();
        vertexBuffer->bufferSize = vertexBufferSize;
        vertexBuffer->bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertexBuffer->bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        vertexBuffer->createBuffer();
        vertexCount = imDrawData->TotalVtxCount;
        vertexBuffer->map();
    }

    // Index buffer
    if ((indexBuffer->buffer == VK_NULL_HANDLE) ||  (indexCount < imDrawData->TotalIdxCount)) {
        indexBuffer->unmap();
        indexBuffer->destroy();
        indexBuffer->bufferSize = indexBufferSize;
        indexBuffer->bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBuffer->bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        indexBuffer->createBuffer();
        indexCount = imDrawData->TotalIdxCount;
        indexBuffer->map();
    }

    // Upload data
    ImDrawVert *vtxDst = (ImDrawVert *) vertexBuffer->mapped;
    ImDrawIdx *idxDst = (ImDrawIdx *) indexBuffer->mapped;

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList *cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    // Flush to make writes visible to GPU
    vertexBuffer->flush();
    indexBuffer->flush();
}

void GUI::drawNewFrame(VkCommandBuffer commandBuffer) {
    ImGuiIO& io = ImGui::GetIO();

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipelineLayout, 0, 1, &descriptor.descriptorSets[0], 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipeline);


    // UI scale and translate via push constants
    pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstBlock.translate = glm::vec2(-1.0f);
    vkCmdPushConstants(commandBuffer, arPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

    // Render commands
    ImDrawData* imDrawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    if (imDrawData->CmdListsCount > 0) {

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

void GUI::drawNewFrame(VkRenderPass renderPass, std::vector<VkFramebuffer> framebuffers) {

}

void GUI::setSettings(std::vector<ArGuiSliderMeshGenerator> _settings) {
    settings = std::move(_settings);
}

std::vector<ArGuiSliderMeshGenerator> GUI::getSettings(){
    return settings;
}

/*
ImGuiIO &io = ImGui::GetIO();

commandBuffers->startRecordCommand(renderPass, std::move(framebuffers));
commandBuffers->bindDescriptorSets(arPipeline.pipelineLayout, descriptor.descriptorSets[0]);
commandBuffers->bindPipeline(arPipeline.pipeline);

// UI scale and translate via push constants
pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
pushConstBlock.translate = glm::vec2(-1.0f);
commandBuffers->pushConstants(arPipeline.pipelineLayout, sizeof(pushConstBlock), pushConstBlock);

// Render commands
ImDrawData* imDrawData = ImGui::GetDrawData();
uint32_t vertexOffset = 0;
uint32_t indexOffset = 0;

if (imDrawData->CmdListsCount > 0) {

    VkDeviceSize offsets[1] = { 0 };
    commandBuffers->vertexBuffer(vertexBuffer->buffer, offsets);
    commandBuffers->indexBuffer(indexBuffer->buffer);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
    {
        const ImDrawList* cmd_list = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
            commandBuffers->setScissor((uint32_t) pcmd->ClipRect.x, (uint32_t)pcmd->ClipRect.y, (uint32_t)pcmd->ClipRect.z,(uint32_t) pcmd->ClipRect.w);
            commandBuffers->drawIndexed(pcmd->ElemCount, indexOffset, vertexOffset);
            indexOffset +=(uint32_t) pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }
}

commandBuffers->endRecord();
*/



//
// Created by magnus on 9/5/21.
//

#ifndef AR_ENGINE_IMGUI_H
#define AR_ENGINE_IMGUI_H


#include <vulkan/vulkan.h>
#include <ar_engine/src/core/VulkanDevice.h>
#include <ar_engine/src/core/VulkanRenderer.h>
#include <ar_engine/src/core/Buffer.h>
#include <imgui.h>


// Options and values to display/toggle from the UI
struct UISettings {
    bool displayModels = true;
    bool displayLogos = true;
    bool displayBackground = true;
    bool animateLight = false;
    float lightSpeed = 0.25f;
    std::array<float, 50> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
    float lightTimer = 0.0f;
};


class ImGUI {
private:
    // Vulkan resources for rendering the UI
    VkSampler sampler{};
    Buffer vertexBuffer;
    Buffer indexBuffer;
    int32_t vertexCount = 0;
    int32_t indexCount = 0;
    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
    VkImage fontImage = VK_NULL_HANDLE;
    VkImageView fontView = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSet descriptorSet{};

    VulkanDevice *device;
    VulkanRenderer *vulkanRenderer;

public:
    // UI params are set via push constants
    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock{};

    UISettings uiSettings;

    explicit ImGUI(VulkanRenderer *vulkanRenderer) : vulkanRenderer(vulkanRenderer) {
        device = vulkanRenderer->vulkanDevice;
        ImGui::CreateContext();
    };

    ~ImGUI() {
        ImGui::DestroyContext();
        // Release all Vulkan resources required for rendering imGui
        vertexBuffer.destroy();
        indexBuffer.destroy();
        vkDestroyImage(device->logicalDevice, fontImage, nullptr);
        vkDestroyImageView(device->logicalDevice, fontView, nullptr);
        vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
        vkDestroySampler(device->logicalDevice, sampler, nullptr);
        vkDestroyPipelineCache(device->logicalDevice, pipelineCache, nullptr);
        vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
        vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
    }

    // Initialize styles, keys, etc.
    void init(float width, float height) {
        // Color scheme
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        // Dimensions
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(width, height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }


    // Initialize all Vulkan resources used by the ui
    void initResources(VkRenderPass renderPass, VkQueue copyQueue, const std::string &shadersPath) {

        ImGuiIO &io = ImGui::GetIO();

        // Create font texture
        unsigned char *fontData;
        int texWidth, texHeight;
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

        // Create Target Image for copy
        VkImageCreateInfo imageInfo = Populate::imageCreateInfo();
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage) != VK_SUCCESS)
            throw std::runtime_error("Failed to create ImGUI image");
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
        VkMemoryAllocateInfo memAllocInfo = Populate::memoryAllocateInfo();
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &fontMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate image IMGUI memory");
        if (vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0))
            throw std::runtime_error("Failed to bind imgui image with its memory");

        // Create Image view to interface with image.
        // Image view
        VkImageViewCreateInfo viewInfo = Populate::imageViewCreateInfo();
        viewInfo.image = fontImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view for imgui image");
        // Staging buffers for font upload
        Buffer stagingBuffer;
        // Copy data to staging buffers
        if (device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &stagingBuffer, uploadSize) != VK_SUCCESS)
            throw std::runtime_error("Failed to create staging buffer");
        stagingBuffer.map();
        memcpy(stagingBuffer.mapped, fontData, uploadSize);
        stagingBuffer.unmap();

        // Transition image layout to transfer and specify copy region
        // Copy buffer data to font image
        VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        // Prepare for transfer
        Utils::setImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Copy
        // Copy staging buffer to image

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = texWidth;
        bufferCopyRegion.imageExtent.height = texHeight;
        bufferCopyRegion.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer.buffer,
                fontImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion
        );

        // Prepare image for shader read
        // Prepare for shader read
        Utils::setImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        device->flushCommandBuffer(copyCmd, copyQueue, true);

        stagingBuffer.destroy();
        // submit cmd buffer and destroy staging buffer

        // Texture sampler
        // Font texture Sampler
        VkSamplerCreateInfo samplerInfo = Populate::samplerCreateInfo();
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        if (vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            throw std::runtime_error("Failed to create texture sampler IMGUI");
        // Descriptor pool
        // Descriptor layout
        // Descriptor set

        // Descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes = {
                Populate::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        };
        VkDescriptorPoolCreateInfo descriptorPoolInfo = Populate::descriptorPoolCreateInfo(poolSizes, 2);
        if (vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool");

        // Descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                Populate::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                     VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorLayout = Populate::descriptorSetLayoutCreateInfo(setLayoutBindings);
        if (vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout");

        // Descriptor set
        VkDescriptorSetAllocateInfo allocInfo = Populate::descriptorSetAllocateInfo(descriptorPool,
                                                                                    &descriptorSetLayout, 1);
        if (vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set");
        VkDescriptorImageInfo fontDescriptor = Populate::descriptorImageInfo(
                sampler,
                fontView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                Populate::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                             &fontDescriptor)
        };
        vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);


        // Pipeline ---------

        // Pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        if (vkCreatePipelineCache(device->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to create Pipeline Cache");

        // Pipeline layout
        // Push constants for UI rendering parameters
        VkPushConstantRange pushConstantRange = Populate::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                                                            sizeof(PushConstBlock), 0);
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Populate::pipelineLayoutCreateInfo(
                &descriptorSetLayout, 1);
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (
                vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
                VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout");

        // Setup graphics pipeline for UI rendering
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
                Populate::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
                                                               VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState =
                Populate::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                               VK_FRONT_FACE_COUNTER_CLOCKWISE);

        // Enable blending
        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.blendEnable = VK_TRUE;
        blendAttachmentState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendState =
                Populate::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState =
                Populate
                ::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo viewportState =
                Populate
                ::pipelineViewportStateCreateInfo(1, 1, 0);

        VkPipelineMultisampleStateCreateInfo multisampleState =
                Populate
                ::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

        std::vector<VkDynamicState> dynamicStateEnables = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState =
                Populate
                ::pipelineDynamicStateCreateInfo(dynamicStateEnables);

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = Populate
        ::pipelineCreateInfo(pipelineLayout,
                             renderPass);

        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();

        // Vertex bindings an attributes based on ImGui vertex definition
        std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                Populate
                ::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
        };
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
                Populate
                ::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert,
                                                                                          pos)),    // Location 0: Position
                Populate
                ::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,
                                                  offsetof(ImDrawVert, uv)),    // Location 1: UV
                Populate
                ::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM,
                                                  offsetof(ImDrawVert, col)),    // Location 0: Color
        };
        VkPipelineVertexInputStateCreateInfo vertexInputState = Populate
        ::pipelineVertexInputStateCreateInfo();
        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

        pipelineCreateInfo.pVertexInputState = &vertexInputState;

        shaderStages[0] = vulkanRenderer->loadShader(shadersPath + "imgui/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = vulkanRenderer->loadShader(shadersPath + "imgui/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        if (vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr,
                                      &pipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");
    }


    // Graphics pipeline

// Starts a new imGui frame and sets up windows and ui elements
    void newFrame(VulkanRenderer *vulkanRenderer, bool updateFrameGraph) {
        ImGui::NewFrame();

        // Init imGui windows and elements

        ImVec4 clear_color = ImColor(114, 144, 154);
        static float f = 0.0f;
        ImGui::TextUnformatted(vulkanRenderer->title.c_str());
        ImGui::TextUnformatted(device->properties.deviceName);

        // Update frame time display
        if (updateFrameGraph) {
            std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
            float frameTime = 1000.0f / (vulkanRenderer->frameTimer * 1000.0f);
            uiSettings.frameTimes.back() = frameTime;
            if (frameTime < uiSettings.frameTimeMin) {
                uiSettings.frameTimeMin = frameTime;
            }
            if (frameTime > uiSettings.frameTimeMax) {
                uiSettings.frameTimeMax = frameTime;
            }
        }

        ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin,
                         uiSettings.frameTimeMax, ImVec2(0, 80));

        ImGui::Text("Camera");
        float pos[3] = {vulkanRenderer->camera.position.x, vulkanRenderer->camera.position.y, vulkanRenderer->camera.position.z};
        float rot[3] = {vulkanRenderer->camera.rotation.x, vulkanRenderer->camera.rotation.y, vulkanRenderer->camera.rotation.z};
        ImGui::InputFloat3("position", pos, "%.3f");
        ImGui::InputFloat3("rotation", rot, "%.3f");

        ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Example settings");
        ImGui::Checkbox("Render models", &uiSettings.displayModels);
        ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
        ImGui::Checkbox("Display background", &uiSettings.displayBackground);
        ImGui::Checkbox("Animate light", &uiSettings.animateLight);
        ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow();

        // Render to generate draw buffers
        ImGui::Render();
    }


// Update vertex and index buffer containing the imGui elements when required
    void updateBuffers() {
        ImDrawData *imDrawData = ImGui::GetDrawData();

        // Note: Alignment is done inside buffer creation
        VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
            return;
        }

        // Update buffers only if vertex or index count has been changed compared to current buffer size

        // Vertex buffer
        if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
            vertexBuffer.unmap();
            vertexBuffer.destroy();
            if (VK_SUCCESS !=
                device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                     &vertexBuffer, vertexBufferSize))
                throw std::runtime_error("Failed to create vertex Buffer");
            vertexCount = imDrawData->TotalVtxCount;
            vertexBuffer.map();
        }

        // Index buffer
        if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
            indexBuffer.unmap();
            indexBuffer.destroy();
            if (VK_SUCCESS !=
                device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                     &indexBuffer, indexBufferSize))
                throw std::runtime_error("Failed to create index buffer");
            indexCount = imDrawData->TotalIdxCount;
            indexBuffer.map();
        }

        // Upload data
        ImDrawVert *vtxDst = (ImDrawVert *) vertexBuffer.mapped;
        ImDrawIdx *idxDst = (ImDrawIdx *) indexBuffer.mapped;

        for (int n = 0; n < imDrawData->CmdListsCount; n++) {
            const ImDrawList *cmd_list = imDrawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Flush to make writes visible to GPU
        vertexBuffer.flush();
        indexBuffer.flush();
    }

// Draw current imGui frame into a command buffer
    void drawFrame(VkCommandBuffer commandBuffer) {
        ImGuiIO &io = ImGui::GetIO();

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                                nullptr);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        //VkViewport viewport = Populate
        // ::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
        //vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // UI scale and translate via push constants
        pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
        pushConstBlock.translate = glm::vec2(-1.0f);
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock),
                           &pushConstBlock);

        // Render commands
        ImDrawData *imDrawData = ImGui::GetDrawData();
        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;

        if (imDrawData->CmdListsCount > 0) {

            VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

            for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
                const ImDrawList *cmd_list = imDrawData->CmdLists[i];
                for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                    const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
                    VkRect2D scissorRect;
                    scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
                    scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
                    scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
                    scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                    vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                    indexOffset += pcmd->ElemCount;
                }
                vertexOffset += cmd_list->VtxBuffer.Size;
            }
        }
    }

};


#endif //AR_ENGINE_IMGUI_H

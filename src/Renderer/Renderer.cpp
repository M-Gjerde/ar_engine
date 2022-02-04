//
// Created by magnus on 9/4/21.
//

#define TINYOBJLOADER_IMPLEMENTATION

#include "Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void Renderer::prepareRenderer() {
    camera.type = Camera::CameraType::firstperson;
    camera.setPerspective(60.0f, (float) width / (float) height, 0.001f, 1024.0f);
    camera.rotationSpeed = 0.25f;
    camera.movementSpeed = 0.1f;
    camera.setPosition({-6.0f, 7.0f, -5.5f});
    camera.setRotation({-35.0f, -45.0f, 0.0f});

    generateScriptClasses();
    prepareUniformBuffers();

    for (auto &script: scripts) {
        if (script->getType() != "None") {
            script->prepareObject();
        }
    }

}

void Renderer::generateScriptClasses() {

    std::vector<std::string> classNames;

    std::string path = Utils::getScriptsPath();
    for (const auto &entry: std::filesystem::directory_iterator(path)) {
        std::string file = entry.path().generic_string();

        // Delete path from filename
        auto n = file.find(path);
        if (n != std::string::npos)
            file.erase(n, path.length());

        // Ensure we have the header file by looking for .h extension
        std::string extension = file.substr(file.find('.') + 1, file.length());
        if (extension == "h") {
            std::string className = file.substr(0, file.find('.'));
            classNames.emplace_back(className);
        }
    }

    // Also add class names to listbox
    UIOverlay->uiSettings.listBoxNames = classNames;
    scripts.reserve(classNames.size());
    // Create class instances of scripts
    for (auto &className: classNames) {
        scripts.push_back(ComponentMethodFactory::Create(className));
    }

    // Run Once
    Base::SetupVars vars{};
    vars.device = vulkanDevice;
    vars.ui = &UIOverlay->uiSettings;
    vars.renderPass = &renderPass;
    vars.UBCount = swapchain.imageCount;

    for (auto &script: scripts) {
        assert(script);
        script->setup(vars);
    }
    printf("Setup finished\n");
}


void Renderer::viewChanged() {
    updateUniformBuffers();
}


void Renderer::UIUpdate(UISettings uiSettings) {
    //printf("Index: %d, name: %s\n", uiSettings.getSelectedItem(), uiSettings.listBoxNames[uiSettings.getSelectedItem()].c_str());

    camera.setMovementSpeed(uiSettings.movementSpeed);

    for (auto &script: scripts) {
        script->onUIUpdate(uiSettings);
    }

}

void Renderer::addDeviceFeatures() {
    printf("Overriden function\n");
    if (deviceFeatures.fillModeNonSolid) {
        enabledFeatures.fillModeNonSolid = VK_TRUE;
        // Wide lines must be present for line width > 1.0f
        if (deviceFeatures.wideLines) {
            enabledFeatures.wideLines = VK_TRUE;
        }
    }
}

void Renderer::buildCommandBuffers() {
    VkCommandBufferBeginInfo cmdBufInfo = Populate::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = {{0.25f, 0.25f, 0.25f, 1.0f}};;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = Populate::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    const VkViewport viewport = Populate::viewport((float) width, (float) height, 0.0f, 1.0f);
    const VkRect2D scissor = Populate::rect2D(width, height, 0, 0);

    for (uint32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        renderPassBeginInfo.framebuffer = frameBuffers[i];
        (vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        for (auto &script: scripts) {
            if (script->getType() == "Terrain") {
                script->draw(drawCmdBuffers[i], i);
            }
        }

        for (auto &script: scripts) {
            if (script->getType() == "Render") {
                script->draw(drawCmdBuffers[i], i);
            }

        }

        UIOverlay->drawFrame(drawCmdBuffers[i]);

        vkCmdEndRenderPass(drawCmdBuffers[i]);
        CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}


void Renderer::render() {
    draw();

    if (UIOverlay->uiSettings.toggleDepthImage)
        storeDepthFrame();

}

void Renderer::draw() {
    VulkanRenderer::prepareFrame();
    buildCommandBuffers();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

    Base::Render renderData{};
    renderData.camera = &camera;
    renderData.params = (void *) UBOFrag;
    renderData.matrix = (void *) UBOVert;
    renderData.deltaT = frameTimer;
    renderData.index = currentBuffer;


    for (auto &script: scripts) {
        if (script->getType() == "Render") {
            script->updateUniformBufferData(renderData);
        }
    }

    vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]);
    VulkanRenderer::submitFrame();


}


void Renderer::updateUniformBuffers() {
    // Scene
    UBOFrag->objectColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
    UBOFrag->lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    UBOFrag->lightPos = glm::vec4(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
    UBOFrag->viewPos = camera.viewPos;

    UBOVert->projection = camera.matrices.perspective;
    UBOVert->view = camera.matrices.view;
    UBOVert->model = glm::mat4(1.0f);

}

void Renderer::prepareUniformBuffers() {
    // TODO REMEMBER TO CLEANUP
    UBOVert = new UBOMatrix();
    UBOFrag = new FragShaderParams();

    updateUniformBuffers();
}

void Renderer::storeDepthFrame() {
    Buffer buffer;
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               &buffer, (VkDeviceSize) width * height * 4);

    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.imageExtent.height = height;
    bufferImageCopy.imageExtent.width = width;
    bufferImageCopy.imageExtent.depth = 1;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.imageOffset.z = 0;
    bufferImageCopy.imageOffset.y = 0;
    bufferImageCopy.imageOffset.x = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.bufferRowLength = 0;

    VkCommandBuffer cmdBuffer =
            vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, cmdPool, true);

    Utils::setImageLayout(cmdBuffer, depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    vkCmdCopyImageToBuffer(cmdBuffer, depthStencil.image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer.buffer, 1, &bufferImageCopy);

    vulkanDevice->flushCommandBuffer(cmdBuffer, queue, cmdPool);

    buffer.map();


    auto *floatPixels = (float *) malloc(width * height * 4);
    memcpy(floatPixels, buffer.mapped, width * height * sizeof(float));

    auto *uintPixels = (uint8_t *) malloc(width * height);

    float min = 100;
    float max = 0;
    for (int i = 0; i < (width * height); ++i) {
        float pixelData = floatPixels[i];

        if (pixelData < min){
            min = pixelData;
        }

        if (pixelData > max)
            max = pixelData;

        float data = (1.0f - pixelData) * 100000;
        uintPixels[i] = (uint8_t) data;

    }

    stbi_write_png("../depthimage.png", width, height, 1, uintPixels, width);
    buffer.unmap();

    delete floatPixels;
    delete uintPixels;
}

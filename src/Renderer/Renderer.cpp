//
// Created by magnus on 9/4/21.
//

#define TINYOBJLOADER_IMPLEMENTATION

#include "Renderer.h"


void Renderer::prepare() {

}

void Renderer::prepareRenderer() {
    {
        /*
         camera.setPosition(glm::vec3(8.6f, 13.0f, 6.0f));
         camera.setRotation(glm::vec3(-37.5f, 125.0f, 0.0f));
         camera.setRotationSpeed(0.25f);
         camera.setPerspective(45.0f, (float) (width) / (float) height, 0.1f, 256.0f);
   */
        camera.type = Camera::CameraType::lookat;
        camera.setPosition(glm::vec3(0.0f, -0.1f, -6.0f));
        camera.setRotation(glm::vec3(0.0f, -135.0f, 0.0f));
        camera.setPerspective(60.0f, (float) width / (float) height, 0.01f, 256.0f);
        camera.setRotationSpeed(0.25f);


        generateScriptClasses();
        // Prepare the Renderer class
        loadAssets();
        loadCube();
        prepareUniformBuffers();
        setupDescriptors();
        preparePipelines();
    }
}

void Renderer::generateScriptClasses() {

    std::vector<std::string> classNames;

    std::string path = getScriptsPath();
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

    sceneObjects.resize(scripts.size());

    // Run Once
    for (auto &script: scripts) {
        assert(script);
        if (script->getType() == "generator") {
            script->setSceneObject(new SceneObject());
        }
        script->setup();
    }
    printf("Setup finished\n");
}

void Renderer::draw() {
    VulkanRenderer::prepareFrame();

    //buildCommandBuffers();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

    vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]);
    VulkanRenderer::submitFrame();
}


void Renderer::render() {

    for (auto &script: scripts) {
        script->update();
    }

    draw();
}

void Renderer::viewChanged() {
    updateUniformBuffers();
}

void Renderer::UIUpdate(UISettings uiSettings) {
    printf("Clicked: %d \n" ,uiSettings.displayModels);
    printf("Index: %d, name: %s\n", uiSettings.getSelectedItem(), uiSettings.listBoxNames[uiSettings.getSelectedItem()].c_str());

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

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        renderPassBeginInfo.framebuffer = frameBuffers[i];
        (vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        UIOverlay->drawFrame(drawCmdBuffers[i]);

        vkCmdEndRenderPass(drawCmdBuffers[i]);
        CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void Renderer::loadglTFFile(std::string filename) {
    /*
    tinygltf::Model glTFInput;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    this->device = device;

    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

    // Pass some Vulkan resources required for setup and rendering to the glTF model loading class
    gltfModel.vulkanDevice = vulkanDevice;
    gltfModel.copyQueue = queue;

    std::vector<uint32_t> indexBuffer;
    std::vector<glTFModel::Vertex> vertexBuffer;

    if (fileLoaded) {
        gltfModel.loadImages(glTFInput);
        gltfModel.loadMaterials(glTFInput);
        gltfModel.loadTextures(glTFInput);
        const tinygltf::Scene &scene = glTFInput.scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
            gltfModel.loadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
        }
    } else {
        throw std::runtime_error(
                "Could not open the glTF file.\n\nThe file is part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.");
    }

    // Create and upload vertex and index buffer
    // We will be using one single vertex buffer and one single index buffer for the whole glTF scene
    // Primitives (of the glTF model) will then index into these using index offsets

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(glTFModel::Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
    gltfModel.indices.count = static_cast<uint32_t>(indexBuffer.size());

    struct StagingBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertexStaging, indexStaging;

    // Create host visible staging buffers (source)
    (vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBufferSize,
            &vertexStaging.buffer,
            &vertexStaging.memory,
            vertexBuffer.data()));
    // Index data
    (vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            indexBufferSize,
            &indexStaging.buffer,
            &indexStaging.memory,
            indexBuffer.data()));

    // Create device local buffers (target)
    (vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBufferSize,
            &gltfModel.vertices.buffer,
            &gltfModel.vertices.memory));
    (vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBufferSize,
            &gltfModel.indices.buffer,
            &gltfModel.indices.memory));

    // Copy data from staging buffers (host) do device local buffer (gpu)
    VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    VkBufferCopy copyRegion = {};

    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(
            copyCmd,
            vertexStaging.buffer,
            gltfModel.vertices.buffer,
            1,
            &copyRegion);

    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(
            copyCmd,
            indexStaging.buffer,
            gltfModel.indices.buffer,
            1,
            &copyRegion);

    vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

    // Free staging resources
    vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
    vkFreeMemory(device, vertexStaging.memory, nullptr);
    vkDestroyBuffer(device, indexStaging.buffer, nullptr);
    vkFreeMemory(device, indexStaging.memory, nullptr);

 */

}
void Renderer::loadAssets() {
    loadglTFFile(getAssetsPath() + "models/DamagedHelmet/glTF/DamagedHelmet.gltf");
}

void Renderer::setupDescriptors() {
    /*
        This sample uses separate descriptor sets (and layouts) for the matrices and materials (textures)


    std::vector<VkDescriptorPoolSize> poolSizes = {
            Populate::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
            // One combined image sampler per model image/texture
            Populate::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         static_cast<uint32_t>(gltfModel.images.size())),
    };
    // One set for matrices and one per model image/texture
    const uint32_t maxSetCount = static_cast<uint32_t>(gltfModel.images.size()) + 1;
    VkDescriptorPoolCreateInfo descriptorPoolInfo = Populate::descriptorPoolCreateInfo(poolSizes, maxSetCount);
    (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout for passing matrices
    VkDescriptorSetLayoutBinding setLayoutBinding = Populate::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = Populate::descriptorSetLayoutCreateInfo(&setLayoutBinding,
                                                                                                    1);
    (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices));
    // Descriptor set layout for passing material textures
    setLayoutBinding = Populate::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                            VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures));
    // Pipeline layout using both descriptor sets (set 0 = matrices, set 1 = material)
    std::array<VkDescriptorSetLayout, 2> setLayouts = {descriptorSetLayouts.matrices, descriptorSetLayouts.textures};
    VkPipelineLayoutCreateInfo pipelineLayoutCI = Populate::pipelineLayoutCreateInfo(setLayouts.data(),
                                                                                     static_cast<uint32_t>(setLayouts.size()));
    // We will use push constants to push the local matrices of a primitive to the vertex shader
    VkPushConstantRange pushConstantRange = Populate::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4),
                                                                        0);
    // Push constant ranges are part of the pipeline layout
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
    (vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

    // Descriptor set for scene matrices
    VkDescriptorSetAllocateInfo allocInfo = Populate::descriptorSetAllocateInfo(descriptorPool,
                                                                                &descriptorSetLayouts.matrices, 1);
    (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    VkWriteDescriptorSet writeDescriptorSet = Populate::writeDescriptorSet(descriptorSet,
                                                                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                                           &shaderData.buffer.descriptor);
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    // Descriptor sets for materials
    for (auto &image: gltfModel.images) {
        const VkDescriptorSetAllocateInfo allocInfo = Populate::descriptorSetAllocateInfo(descriptorPool,
                                                                                          &descriptorSetLayouts.textures,
                                                                                          1);
        (vkAllocateDescriptorSets(device, &allocInfo, &image.descriptorSet));
        VkWriteDescriptorSet writeDescriptorSet = Populate::writeDescriptorSet(image.descriptorSet,
                                                                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                               0, &image.texture.descriptor);
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }
     */
}

void Renderer::preparePipelines() {

}

// Prepare and initialize uniform buffer containing shader uniforms
void Renderer::prepareUniformBuffers() {
    // Vertex shader uniform buffer block
    (vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &shaderData.buffer,
            sizeof(shaderData.values)));

    // Map persistent
    (shaderData.buffer.map());

    updateUniformBuffers();
}

void Renderer::updateUniformBuffers() {

    shaderData.values.projection = camera.matrices.perspective;
    shaderData.values.model = camera.matrices.view;
    memcpy(shaderData.buffer.mapped, &shaderData.values, sizeof(shaderData.values));
}

void Renderer::loadCube() {

    // Use library to load in model
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string MODEL_PATH = "../objects/standard/cube.obj";


    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }


    float nx, ny, nz = 0.0f; // normal for current triangle
    float vx1, vx2, vx3 = 0.f; // vertex 1
    float vy1, vy2, vy3 = 0.f; // vertex 2
    float vz1, vz2, vz3 = 0.f; // vertex 3


    // Load vertex data // TODO Load index data as well
    for (int i = 0; i < shapes.size(); ++i) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); ++j) {
            Vertex vertex{};
            glm::vec3 normal;

            vertex.pos = {attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]};

            if (!attrib.texcoords.empty())
                vertex.texCoord = {attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1]};

            // calculate normals for plane
            if (!attrib.normals.empty()) {


                vertex.normal = {attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 0],
                                 attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 1],
                                 attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 2]};

            }
        }
    }

}


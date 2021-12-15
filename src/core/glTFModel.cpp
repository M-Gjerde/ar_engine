//
// Created by magnus on 12/10/21.
//

#include <glm/gtc/type_ptr.hpp>
#include <ar_engine/src/tools/Macros.h>
#include <ar_engine/src/Renderer/shaderParams.h>
#include "glTFModel.h"


void glTFModel::Model::loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue, float scale) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error;
    std::string warning;

    this->device = device;

    bool binary = false;
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos) {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
    }

    bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str())
                             : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
        loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
    }

    loadTextureSamplers(gltfModel);
    loadTextures(gltfModel, device, transferQueue);
    loadMaterials(gltfModel);
    extensions = gltfModel.extensionsUsed;

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
    indices.count = static_cast<uint32_t>(indexBuffer.size());

    assert(vertexBufferSize > 0);

    struct StagingBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertexStaging, indexStaging;

    // Create staging buffers
    // Vertex data
    CHECK_RESULT(device->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBufferSize,
            &vertexStaging.buffer,
            &vertexStaging.memory,
            vertexBuffer.data()));
    // Index data
    if (indexBufferSize > 0) {
        CHECK_RESULT(device->createBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indexBufferSize,
                &indexStaging.buffer,
                &indexStaging.memory,
                indexBuffer.data()));
    }

    // Create device local buffers
    // Vertex buffer
    CHECK_RESULT(device->createBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBufferSize,
            &vertices.buffer,
            &vertices.memory));
    // Index buffer
    if (indexBufferSize > 0) {
        CHECK_RESULT(device->createBuffer(
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBufferSize,
                &indices.buffer,
                &indices.memory));
    }

    // Copy from staging buffers
    VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkBufferCopy copyRegion = {};

    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

    if (indexBufferSize > 0) {
        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
    }

    device->flushCommandBuffer(copyCmd, transferQueue, true);

    vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
    if (indexBufferSize > 0) {
        vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
        vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
    }


}

void glTFModel::drawNode(Node *node, VkCommandBuffer commandBuffer) {
    for (Primitive primitive: model.primitives) {
        vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }

    for (auto &child: node->children) {
        drawNode(child, commandBuffer);
    }
}

void glTFModel::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &descriptors[i], 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertices.buffer, offsets);
    if (model.indices.buffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(commandBuffer, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
    for (auto &node: model.nodes) {
        drawNode(node, commandBuffer);
    }
}


void glTFModel::Model::loadNode(glTFModel::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex,
                                const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer,
                                std::vector<Vertex> &vertexBuffer, float globalscale) {

    auto *newNode = new Node{};
    newNode->index = nodeIndex;
    newNode->parent = parent;
    newNode->name = node.name;
    newNode->skinIndex = node.skin;
    newNode->matrix = glm::mat4(1.0f);

    // Generate local node matrix
    auto translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
    }
    if (node.matrix.size() == 16) {
        newNode->matrix = glm::make_mat4x4(node.matrix.data());
    };

    // Node with children
    if (node.children.size() > 0) {
        for (size_t i = 0; i < node.children.size(); i++) {
            loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer,
                     globalscale);
        }
    }

    // Node contains mesh data
    if (node.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        for (auto &primitive: mesh.primitives) {
            uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            glm::vec3 posMin{};
            glm::vec3 posMax{};
            bool hasSkin = false;
            bool hasIndices = primitive.indices > -1;

            // Vertices
            {
                const float *bufferPos = nullptr;
                const float *bufferNormals = nullptr;
                const float *bufferTexCoordSet0 = nullptr;
                const float *bufferTexCoordSet1 = nullptr;

                int posByteStride;
                int normByteStride;
                int uv0ByteStride;
                int uv1ByteStride;
                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find(
                        "POSITION")->second];
                const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
                bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[
                        posAccessor.byteOffset + posView.byteOffset]));
                posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                vertexCount = static_cast<uint32_t>(posAccessor.count);
                posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float))
                                                                : tinygltf::GetComponentSizeInBytes(
                                TINYGLTF_TYPE_VEC3);

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find(
                            "NORMAL")->second];
                    const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                    bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[
                            normAccessor.byteOffset + normView.byteOffset]));
                    normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) /
                                                                          sizeof(float))
                                                                       : tinygltf::GetComponentSizeInBytes(
                                    TINYGLTF_TYPE_VEC3);
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find(
                            "TEXCOORD_0")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[
                            uvAccessor.byteOffset + uvView.byteOffset]));
                    uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                                  : tinygltf::GetComponentSizeInBytes(
                                    TINYGLTF_TYPE_VEC2);
                }
                if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
                    const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find(
                            "TEXCOORD_1")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[
                            uvAccessor.byteOffset + uvView.byteOffset]));
                    uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                                  : tinygltf::GetComponentSizeInBytes(
                                    TINYGLTF_TYPE_VEC2);
                }

                for (size_t v = 0; v < posAccessor.count; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(
                            bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride])
                                                  : glm::vec3(0.0f);
                    vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride])
                                                  : glm::vec3(0.0f);
                    vertexBuffer.push_back(vert);
                }
            }

            if (hasIndices) {
                const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices
                                                                                            : 0];
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);
                const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const uint32_t *buf = static_cast<const uint32_t *>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const uint16_t *buf = static_cast<const uint16_t *>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const uint8_t *buf = static_cast<const uint8_t *>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!"
                                  << std::endl;
                        return;
                }
            }
            Primitive newPrimitive(indexStart, indexCount, vertexCount);
            primitives.push_back(newPrimitive);
        }
    }

    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
    linearNodes.push_back(newNode);

}

void glTFModel::Model::loadTextureSamplers(tinygltf::Model &gltfModel) {
    for (const tinygltf::Sampler &smpl: gltfModel.samplers) {
        Texture::TextureSampler sampler{};
        sampler.minFilter = getVkFilterMode(smpl.minFilter);
        sampler.magFilter = getVkFilterMode(smpl.magFilter);
        sampler.addressModeU = getVkWrapMode(smpl.wrapS);
        sampler.addressModeV = getVkWrapMode(smpl.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        textureSamplers.push_back(sampler);
    }
}

void glTFModel::Model::loadTextures(tinygltf::Model &gltfModel, VulkanDevice *device, VkQueue transferQueue) {
    for (tinygltf::Texture &tex: gltfModel.textures) {
        tinygltf::Image image = gltfModel.images[tex.source];
        Texture::TextureSampler sampler{};
        if (tex.sampler == -1) {
            // No sampler specified, use a default one
            sampler.magFilter = VK_FILTER_LINEAR;
            sampler.minFilter = VK_FILTER_LINEAR;
            sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else {
            sampler = textureSamplers[tex.sampler];
        }
        Texture2D texture2D;
        texture2D.fromglTfImage(image, sampler, device, transferQueue);
        textures.push_back(texture2D);
    }


}

void glTFModel::Model::loadMaterials(tinygltf::Model &gltfModel) {
    for (tinygltf::Material &mat: gltfModel.materials) {
        glTFModel::Material material{};
        if (mat.values.find("baseColorTexture") != mat.values.end()) {
            material.baseColorTexture = &textures[mat.additionalValues["baseColorTexture"].TextureIndex()];
            material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
            textureIndices.baseColor = 0;

        }
        if (mat.values.find("baseColorFactor") != mat.values.end()) {
            material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
        }

        if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
            textureIndices.normalMap = mat.additionalValues["normalTexture"].TextureIndex();
            material.normalTexture = &textures[mat.additionalValues["normalTexture"].TextureIndex()];
            material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
        }

        materials.push_back(material);
    }
}

VkSamplerAddressMode glTFModel::Model::getVkWrapMode(int32_t wrapMode) {
    switch (wrapMode) {
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
}

VkFilter glTFModel::Model::getVkFilterMode(int32_t filterMode) {
    switch (filterMode) {
        case 9728:
            return VK_FILTER_NEAREST;
        case 9729:
            return VK_FILTER_LINEAR;
        case 9984:
            return VK_FILTER_NEAREST;
        case 9985:
            return VK_FILTER_NEAREST;
        case 9986:
            return VK_FILTER_LINEAR;
        case 9987:
            return VK_FILTER_LINEAR;
        default:
            std::cerr << "Sampler filter defaulted to VK_FILTER_LINEAR" << std::endl;
            return VK_FILTER_LINEAR;
    }
}


glTFModel::glTFModel() {
    printf("glTFModel Constructor\n");
}

void glTFModel::prepareUniformBuffers(uint32_t count) {
    uniformBuffers.resize(count);

    for (auto &uniformBuffer: uniformBuffers) {

        device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &uniformBuffer.model, sizeof(UBOMatrices));

        device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &uniformBuffer.shaderValues, sizeof(ShaderValuesParams));

        device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &uniformBuffer.selection, sizeof(float));

    }

}

void glTFModel::updateUniformBufferData(uint32_t index, void *params, void *matrix, void *selection) {
    UniformBufferSet currentUB = uniformBuffers[index];

    currentUB.model.map();
    memcpy(currentUB.model.mapped, matrix, sizeof(SimpleUBOMatrix));
    currentUB.model.unmap();

    currentUB.shaderValues.map();
    memcpy(currentUB.shaderValues.mapped, params, sizeof(FragShaderParams));
    currentUB.shaderValues.unmap();

    currentUB.selection.map();

    char* val = static_cast<char *>(selection);
    float f = (float) atoi(val);
    memcpy(currentUB.selection.mapped, &f, sizeof(float));
    currentUB.selection.unmap();

}

glTFModel::Primitive::Primitive(uint32_t _firstIndex, uint32_t indexCount, uint32_t vertexCount) {
    this->firstIndex = _firstIndex;
    this->indexCount = indexCount;
    this->vertexCount = vertexCount;
    hasIndices = indexCount > 0;

}

void glTFModel::Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max) {

}

void glTFModel::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr},
            //{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    CHECK_RESULT(
            vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));
}

void glTFModel::createDescriptors(uint32_t count) {
    descriptors.resize(count);

    /**
     * Create Descriptor Pool
     */

    uint32_t uniformDescriptorCount = 3 * count + model.nodes.size();
    uint32_t imageDescriptorSamplerCount = 3 * count * 3;
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         uniformDescriptorCount},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageDescriptorSamplerCount},

    };
    VkDescriptorPoolCreateInfo poolCreateInfo = Populate::descriptorPoolCreateInfo(poolSizes,
                                                                                   count + model.nodes.size());
    CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &poolCreateInfo, nullptr, &descriptorPool));


    /**
     * Create Descriptor Sets
     */
    for (auto i = 0; i < descriptors.size(); i++) {

        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = descriptorPool;
        descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &descriptors[i]));

        std::array<VkWriteDescriptorSet, 4> writeDescriptorSets{};

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].dstSet = descriptors[i];
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].pBufferInfo = &uniformBuffers[i].model.descriptorBufferInfo;

/*
        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].dstSet = descriptors[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].pBufferInfo = &uniformBuffers[i].shaderValues.descriptorBufferInfo;*/

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].dstSet = descriptors[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].pImageInfo = &model.textures[model.textureIndices.baseColor].descriptor;

        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[2].descriptorCount = 1;
        writeDescriptorSets[2].dstSet = descriptors[i];
        writeDescriptorSets[2].dstBinding = 2;
        writeDescriptorSets[2].pImageInfo = &model.textures[model.textureIndices.normalMap].descriptor;

        writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[3].descriptorCount = 1;
        writeDescriptorSets[3].dstSet = descriptors[i];
        writeDescriptorSets[3].dstBinding = 3;
        writeDescriptorSets[3].pBufferInfo = &uniformBuffers[i].selection.descriptorBufferInfo;

        vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, NULL);
    }


    // Model node (matrices)
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        CHECK_RESULT(
                vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorSetLayoutCI, nullptr,
                                            &descriptorSetLayoutNode));

        // Per-Node descriptor set
        for (auto &node: model.nodes) {
            setupNodeDescriptorSet(node);
        }
    }

}

void glTFModel::setupNodeDescriptorSet(glTFModel::Node *node) {
    if (node->mesh) {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = descriptorPool;
        descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayoutNode;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        CHECK_RESULT(
                vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo,
                                         &node->mesh->uniformBuffer.descriptorSet));

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

        vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto &child: node->children) {
        setupNodeDescriptorSet(child);
    }
}


void glTFModel::createPipeline(VkRenderPass renderPass, std::vector<VkPipelineShaderStageCreateInfo> shaderStages) {


    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = Populate::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationStateCI = Populate::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    rasterizationStateCI.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachmentState = Populate::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI = Populate::pipelineColorBlendStateCreateInfo(1,
                                                                                                        &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
            Populate::pipelineDepthStencilStateCreateInfo(VK_TRUE,
                                                          VK_TRUE,
                                                          VK_COMPARE_OP_LESS);
    depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCI.stencilTestEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;


    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());


    // Pipeline layout
    const std::vector<VkDescriptorSetLayout> setLayouts = {
            descriptorSetLayout, descriptorSetLayoutNode
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutCI.pSetLayouts = setLayouts.data();
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
    CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCI, nullptr, &pipelineLayout));


    // Vertex bindings an attributes
    VkVertexInputBindingDescription vertexInputBinding = {0, sizeof(glTFModel::Model::Vertex),
                                                          VK_VERTEX_INPUT_RATE_VERTEX};
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    0},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT,    sizeof(float) * 3},
            {2, 0, VK_FORMAT_R32G32_SFLOAT,       sizeof(float) * 6},
            {3, 0, VK_FORMAT_R32G32_SFLOAT,       sizeof(float) * 8},
            {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 10},
            {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 14}
    };
    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Pipelines
    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.layout = pipelineLayout;
    pipelineCI.renderPass = renderPass;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


    CHECK_RESULT(vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCI, nullptr, &pipeline));

    for (auto shaderStage: shaderStages) {
        vkDestroyShaderModule(device->logicalDevice, shaderStage.module, nullptr);
    }


}

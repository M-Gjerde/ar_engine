//
// Created by magnus on 12/10/21.
//

#include <glm/gtc/type_ptr.hpp>
#include <ar_engine/src/tools/Macros.h>
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

    loadSkins(gltfModel);

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

void glTFModel::Model::loadSkins(tinygltf::Model &gltfModel) {
    for (tinygltf::Skin &source: gltfModel.skins) {
        Skin *newSkin = new Skin{};
        newSkin->name = source.name;

        // Find skeleton root node
        if (source.skeleton > -1) {
            newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
        }

        // Find joint nodes
        for (int jointIndex: source.joints) {
            Node *node = nodeFromIndex(jointIndex);
            if (node) {
                newSkin->joints.push_back(nodeFromIndex(jointIndex));
            }
        }

        // Get inverse bind matrices from buffer
        if (source.inverseBindMatrices > -1) {
            const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
            const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
            newSkin->inverseBindMatrices.resize(accessor.count);
            memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                   accessor.count * sizeof(glm::mat4));
        }

        skins.push_back(newSkin);
    }
}

void glTFModel::Model::drawNode(Node *node, VkCommandBuffer commandBuffer) {
    printf("Drawing node\n");
    if (node->mesh) {
        for (Primitive *primitive: node->mesh->primitives) {
            vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
    }
    for (auto &child: node->children) {
        drawNode(child, commandBuffer);
    }
}

void glTFModel::Model::draw(VkCommandBuffer commandBuffer) {
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    for (auto &node: nodes) {
        drawNode(node, commandBuffer);
    }
}

glTFModel::Node *glTFModel::Model::findNode(Node *parent, uint32_t index) {
    Node *nodeFound = nullptr;
    if (parent->index == index) {
        return parent;
    }
    for (auto &child: parent->children) {
        nodeFound = findNode(child, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

glTFModel::Node *glTFModel::Model::nodeFromIndex(uint32_t index) {
    Node *nodeFound = nullptr;
    for (auto &node: nodes) {
        nodeFound = findNode(node, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

void glTFModel::Model::loadNode(glTFModel::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex,
                                const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer,
                                std::vector<Vertex> &vertexBuffer, float globalscale) {

    glTFModel::Node *newNode = new Node{};
    newNode->index = nodeIndex;
    newNode->parent = parent;
    newNode->name = node.name;
    newNode->skinIndex = node.skin;
    newNode->matrix = glm::mat4(1.0f);

    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
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
        Mesh *newMesh = new Mesh(device, newNode->matrix);
        for (size_t j = 0; j < mesh.primitives.size(); j++) {
            const tinygltf::Primitive &primitive = mesh.primitives[j];
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
                const void *bufferJoints = nullptr;
                const float *bufferWeights = nullptr;

                int posByteStride;
                int normByteStride;
                int uv0ByteStride;
                int uv1ByteStride;
                int jointByteStride;
                int weightByteStride;

                int jointComponentType;

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

                // Skinning
                // Joints
                if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find(
                            "JOINTS_0")->second];
                    const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
                    bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset +
                                                                          jointView.byteOffset]);
                    jointComponentType = jointAccessor.componentType;
                    jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) /
                                                                             tinygltf::GetComponentSizeInBytes(
                                                                                     jointComponentType))
                                                                          : tinygltf::GetComponentSizeInBytes(
                                    TINYGLTF_TYPE_VEC4);
                }

                if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find(
                            "WEIGHTS_0")->second];
                    const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
                    bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[
                            weightAccessor.byteOffset + weightView.byteOffset]));
                    weightByteStride = weightAccessor.ByteStride(weightView) ? (
                            weightAccessor.ByteStride(weightView) / sizeof(float))
                                                                             : tinygltf::GetComponentSizeInBytes(
                                    TINYGLTF_TYPE_VEC4);
                }

                hasSkin = (bufferJoints && bufferWeights);

                for (size_t v = 0; v < posAccessor.count; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(
                            bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride])
                                                  : glm::vec3(0.0f);
                    vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride])
                                                  : glm::vec3(0.0f);

                    if (hasSkin) {
                        switch (jointComponentType) {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                                const uint16_t *buf = static_cast<const uint16_t *>(bufferJoints);
                                vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
                                break;
                            }
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                                const uint8_t *buf = static_cast<const uint8_t *>(bufferJoints);
                                vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
                                break;
                            }
                            default:
                                // Not supported by spec
                                std::cerr << "Joint component type " << jointComponentType << " not supported!"
                                          << std::endl;
                                break;
                        }
                    } else {
                        vert.joint0 = glm::vec4(0.0f);
                    }
                    vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
                    // Fix for all zero weights
                    if (glm::length(vert.weight0) == 0.0f) {
                        vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
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
            Primitive *newPrimitive = new Primitive(indexStart, indexCount, vertexCount);
            newPrimitive->setBoundingBox(posMin, posMax);
            newMesh->primitives.push_back(newPrimitive);
        }

        newNode->mesh = newMesh;
    }
    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
    linearNodes.push_back(newNode);

}

glTFModel::glTFModel() {
    printf("glTFModel Constructor\n");
}

glTFModel::Primitive::Primitive(uint32_t _firstIndex, uint32_t indexCount, uint32_t vertexCount) {
    firstIndex = _firstIndex;
    this->indexCount = indexCount;
    this->vertexCount = vertexCount;
    hasIndices = indexCount > 0;

}

void glTFModel::Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max) {

}

glTFModel::Mesh::Mesh(VulkanDevice *device, glm::mat4 matrix) {
    this->device = device;
    this->uniformBlock.matrix = matrix;
    CHECK_RESULT(device->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(uniformBlock),
            &uniformBuffer.buffer,
            &uniformBuffer.memory,
            &uniformBlock));
    CHECK_RESULT(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0,
                             &uniformBuffer.mapped));
    uniformBuffer.descriptor = {uniformBuffer.buffer, 0, sizeof(uniformBlock)};

}

glTFModel::Mesh::~Mesh() {
    vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
    for (Primitive *p: primitives)
        delete p;
}

void glTFModel::Node::update() {
    if (mesh) {
        glm::mat4 m = getMatrix();
        memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));

    }

    for (auto &child: children) {
        child->update();
    }
}

glm::mat4  glTFModel::Node::getMatrix() {
    glm::mat4 m = localMatrix();
    Node *p = parent;
    while (p) {
        m = p->localMatrix() * m;
        p = p->parent;
    }
    return m;
}

glm::mat4 glTFModel::Node::localMatrix() {
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) *
           matrix;
}
//
// Created by magnus on 7/7/20.
//

#include <cstring>
#include "../headers/Mesh.hpp"

Mesh::Mesh() = default;

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
           VkCommandPool transferCommandPool, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, int newTexId) {
    physicalDevice = newPhysicalDevice;
    device = newDevice;
    vertexCount = vertices->size();
    indexCount = indices->size();
    createVertexBuffer(transferQueue, transferCommandPool, vertices);
    createIndexBuffer(transferQueue, transferCommandPool, indices);

    model.model = glm::mat4(1.0f);
    texId = newTexId;
}

[[maybe_unused]] [[maybe_unused]] int Mesh::getVertexCount() const {
    return vertexCount;
}

VkBuffer Mesh::getVertexBuffer() {
    return vertexBuffer;
}

int Mesh::getTexId(){
    return texId;
}


void Mesh::destroyBuffers() {
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}


void Mesh::createVertexBuffer(VkQueue transferQueue,
                              VkCommandPool transferCommandPool, std::vector<Vertex> *vertices) {

    //Get size of buffer needed for vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Create Staging buffer and allocate memory to it
    createBuffer(physicalDevice,
                 device,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer,
                 &stagingBufferMemory);


    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                              // 1. Create pointer to a point in normal memory
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0,
                &data);                                                     // 2. "Map" the vertex buffer memory to that point
    memcpy(data, vertices->data(),
           (size_t) bufferSize);                                            // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(device, stagingBufferMemory);                              // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not the CPU
    createBuffer(physicalDevice,
                 device,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &vertexBuffer,
                 &vertexBufferMemory
    );

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

    // Destroy temporary staging buffer -> cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices) {
    //Get size of buffer needed for indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

    // temporary buffer to "stage" index data before transferring to GPU
    VkDeviceMemory stagingBufferMemory;
    VkBuffer stagingBuffer;

    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO INDEX BUFFER
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices->data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create buffer for INDEX data on GPU access only area
    createBuffer(physicalDevice,
                 device,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &indexBuffer,
                 &indexBufferMemory);

    // Copy from staging buffer to GPU access buffer
    copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

    // Destroy + release staging buffer resource
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

int Mesh::getIndexCount() const {
    return indexCount;
}

VkBuffer Mesh::getIndexBuffer() {
    return indexBuffer;
}

void Mesh::setModel(glm::mat4 newModel) {
    model.model = newModel;
}

Model Mesh::getModel()
{
    return model;
}


Mesh::~Mesh() = default;

//
// Created by magnus on 10/8/20.
//

#include "Mesh.h"

#include <utility>
#include <cstring>


Mesh::Mesh(MainDevice mainDevice, StandardModel *standardModel, std::vector<ArBuffer> newArBuffer) : Buffer(
        mainDevice) {

    device = mainDevice.device;
    modelInfo = *standardModel;

    // Use ArBuffer struct to use less memory than by just using standard Model struct
    vertexBuffer = newArBuffer[0];
    indexBuffer = newArBuffer[1];

    // Populate ArBuffers
    createVertexBuffer();
    createIndexBuffer();

    // Copy data back to standard model.
    standardModel->vertexBuffer = vertexBuffer.buffer;
    standardModel->indexBuffer = indexBuffer.buffer;
    standardModel->indexCount = meshIndices.size();
}


void Mesh::cleanUp() {
    vkFreeMemory(device, vertexBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);

    vkFreeMemory(device, indexBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
}


void Mesh::createVertexBuffer() {

    // Temporary buffer to "stage" vertex data before transferring to GPU
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBuffer.bufferSize = vertexBuffer.bufferSize;

    // Create Staging buffer and allocate memory to it
    createBuffer(&stagingBuffer);


    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                                                         // 1. Create pointer to a point in normal memory
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, vertexBuffer.bufferSize, 0,
                &data);     // 2. "Map" the vertex buffer memory to that point
    memcpy(data, modelInfo.vertices.data(),
           vertexBuffer.bufferSize);                                                   // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(device,
                  stagingBuffer.bufferMemory);                                                  // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not the CPU
    createBuffer(&vertexBuffer);

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(modelInfo, stagingBuffer, vertexBuffer);

    // Destroy temporary staging buffer -> cleanup
    vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);

}

void Mesh::createIndexBuffer() {
    // Temporary buffer to "stage" vertex data before transferring to GPU
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBuffer.bufferSize = indexBuffer.bufferSize;

    // Create Staging buffer and allocate memory to it
    createBuffer(&stagingBuffer);


    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                                                         // 1. Create pointer to a point in normal memory
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, indexBuffer.bufferSize, 0, &data);     // 2. "Map" the vertex buffer memory to that point
    memcpy(data, modelInfo.indices.data(), indexBuffer.bufferSize);                                                   // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(device, stagingBuffer.bufferMemory);                                                  // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not the CPU
    createBuffer(&indexBuffer);

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(modelInfo, stagingBuffer, indexBuffer);

    // Destroy temporary staging buffer -> cleanup
    vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);

}

const glm::mat4 & Mesh::getModel() const {
    return model1.model;
}

void Mesh::setModel(const glm::mat4 &_model) {
    model1.model = _model;
}

Mesh::~Mesh() = default;
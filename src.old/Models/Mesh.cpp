//
// Created by magnus on 10/8/20.
//

#include "Mesh.h"






Mesh::Mesh(MainDevice mainDevice, ArModel *standardModel, std::vector<ArBuffer> newArBuffer) : Buffer(
        mainDevice) {

    device = mainDevice.device;
    modelInfo = *standardModel;

    // Use ArBuffer struct to use less memory than by just using standard Model struct
    indexBuffer = newArBuffer[1];
    vertexBuffer = newArBuffer[0];
    // Populate ArBuffers
    createVertexBuffer();
    createIndexBuffer();

    // Copy data back to standard model.
    standardModel->indexBuffer = indexBuffer.buffer;
    standardModel->vertexBuffer = vertexBuffer.buffer;
    standardModel->indexBufferMemory = indexBuffer.bufferMemory;
    standardModel->vertexBufferMemory = vertexBuffer.bufferMemory;

    //standardModel->indexCount = meshIndices.size();
}


void Mesh::createVertexBuffer() {

    // Temporary buffer to "stage" vertex data before transferring to GPU
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBuffer.bufferSize = vertexBuffer.bufferSize;
    stagingBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create Staging buffer and allocate memory to it
    createBuffer(&stagingBuffer);

    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                                                         // 1. Create pointer to a point in normal memory
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, vertexBuffer.bufferSize, 0,
                &data);                                                                                 // 2. "Map" the vertex buffer memory to that point
    memcpy(data, modelInfo.vertices.data(),
           vertexBuffer.bufferSize);                                                                    // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(device,
                  stagingBuffer.bufferMemory);                                                          // 4. Unmap the vertex buffer memory

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
    stagingBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create Staging buffer and allocate memory to it
    createBuffer(&stagingBuffer);


    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                                                         // 1. Create pointer to a point in normal memory
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, indexBuffer.bufferSize, 0,
                &data);     // 2. "Map" the vertex buffer memory to that point
    memcpy(data, modelInfo.indices.data(),
           indexBuffer.bufferSize);                                                   // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(device,
                  stagingBuffer.bufferMemory);                                                  // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not the CPU
    createBuffer(&indexBuffer);

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(modelInfo, stagingBuffer, indexBuffer);

    // Destroy temporary staging buffer -> cleanup
    vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);

}

Mesh::~Mesh() = default;
//
// Created by magnus on 10/8/20.
//

#include "Mesh.h"

void Mesh::createVertexBuffer(ArBuffer *arBuffer) {

    //Get size of buffer needed for vertices


    // Temporary buffer to "stage" vertex data before transferring to GPU
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Create Staging buffer and allocate memory to it
    createBuffer(&stagingBuffer);
/*

    // MAP MEMORY TO STAGING BUFFER
    void *data;                                                              // 1. Create pointer to a point in normal memory
    vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, arBuffer, 0,
                &data);                                                     // 2. "Map" the vertex buffer memory to that point
    memcpy(data, vertices->data(),
           (size_t) arBuffer);                                            // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);                              // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not the CPU
    createBuffer(mainDevice,
                 arBuffer,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &vertexBuffer,
                 &vertexBufferMemory
    );

    // Copy staging buffer to vertex buffer on GPU
    Utils::copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, arBuffer);

    // Destroy temporary staging buffer -> cleanup
    vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
*/
}

void Mesh::createIndexBuffer() {



}

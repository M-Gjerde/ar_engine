//
// Created by magnus on 10/5/21.
//

#include <ar_engine/src/tools/Macros.h>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>
#include "vkMyModel.h"

void vkMyModel::destroy(VkDevice device) {

}

void vkMyModel::loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue, float scale) {

}

void vkMyModel::setMesh(std::vector<Vertex> vertexBuffer, std::vector<uint32_t> indexBuffer) {


}

void vkMyModel::useStagingBuffer(std::vector<Vertex> vertexBuffer, std::vector<uint32_t> indexBuffer) {

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
    indices.count = indexBuffer.size();
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

    device->flushCommandBuffer(copyCmd, device->transferQueue, true);

    vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
    if (indexBufferSize > 0) {
        vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
        vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
    }

}

void vkMyModel::draw(VkCommandBuffer commandBuffer) {
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, indices.count, 1, 0, 0, 0);
}

void vkMyModel::getSceneDimensions() {

}

void vkMyModel::generateSquare() {
    // 16*16 mesh as our ground
    // Get square size from input
    int xSize = 12;
    int zSize = 12;

    int v = 0;
    auto *pn = new PerlinNoise(123);

    uint32_t vertexCount = (xSize + 1) * (zSize + 1);
    uint32_t indexCount = xSize * zSize * 6;
    // Alloc memory for vertices and indices
    std::vector<Vertex> vertices(vertexCount);
    vertices.resize(vertexCount);
    for (int z = 0; z <= zSize; ++z) {
        for (int x = 0; x <= xSize; ++x) {
            Vertex vertex{};

            // Use the grid size to determine the perlin noise image.
            double i = (double) x / ((double) xSize);
            double j = (double) z / ((double) zSize);
            double n = pn->noise(100 * i, 100 * j, 0.8);
            vertex.pos = glm::vec3(x, n, z);
            vertices[v] = vertex;
            vertices[v] = vertex;
            v++;

            // calculate Normals for every 3 vertices
            if ((v % 3) == 0) {
                glm::vec3 A = vertices[v - 2].pos;
                glm::vec3 B = vertices[v - 1].pos;
                glm::vec3 C = vertices[v].pos;
                glm::vec3 AB = B - A;
                glm::vec3 AC = C - A;

                glm::vec3 normal = glm::cross(AC, AB);
                normal = glm::normalize(normal);
                // Give normal to last three vertices
                vertices[v - 2].normal = normal;
                vertices[v - 1].normal = normal;
                vertices[v].normal = normal;
            }
        }
    }
    std::vector<uint32_t> indices(indexCount);
    indices.resize(indexCount);
    int tris = 0;
    int vert = 0;
    for (int z = 0; z < zSize; ++z) {
        for (int x = 0; x < xSize; ++x) {
            // One quad
            indices[tris + 0] = vert;
            indices[tris + 1] = vert + 1;
            indices[tris + 2] = vert + xSize + 1;
            indices[tris + 3] = vert + 1;
            indices[tris + 4] = vert + xSize + 2;
            indices[tris + 5] = vert + xSize + 1;

            vert++;
            tris += 6;
        }
        vert++;
    }

    useStagingBuffer(vertices, indices);
}

void vkMyModel::load(VulkanDevice *pDevice) {
    this->device = pDevice;
    generateSquare();
    printf("Finished generating mesh\n");
}

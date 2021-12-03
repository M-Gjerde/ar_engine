//
// Created by magnus on 10/5/21.
//

#ifndef AR_ENGINE_VKMYMODEL_H
#define AR_ENGINE_VKMYMODEL_H


#include <glm/glm.hpp>
#include "VulkanDevice.h"
#include "Texture.h"

class vkMyModel {

public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 joint0;
        glm::vec4 weight0;
    };
    vkMyModel* model = this;
    VulkanDevice *device{};

    struct Vertices {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory;
    } vertices;

    struct Indices {
        int count;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory;
    } indices;

    glm::mat4 aabb{};

    std::vector<Texture> textures;
    std::vector<std::string> extensions;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
    } dimensions;

    vkMyModel();
    void destroy(VkDevice device);

    void loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue, float scale = 1.0f);


    void createDescriptors();

    void createPipeline();

    void draw(VkCommandBuffer commandBuffer);

    void useStagingBuffer(Vertex *_vertices, uint32_t vertexCount, unsigned int *_indices, uint32_t indexCount);
};


#endif //AR_ENGINE_VKMYMODEL_H

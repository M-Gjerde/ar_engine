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


    void load(VulkanDevice *pDevice);

    void createDescriptors();
    void createPipeline();
    void setMesh(std::vector<Vertex> vertexBuffer, std::vector<uint32_t> indexBuffer);
    void useStagingBuffer(std::vector<Vertex> vertexBuffer, std::vector<uint32_t> indexBuffer);
    void draw(VkCommandBuffer commandBuffer);

    VulkanDevice *device;
    VkQueue queue;

    struct Vertices {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory;
        } vertices;

        struct Indices {
            int count;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory;
        } indices;

        glm::mat4 aabb;


        std::vector<Texture> textures;
        std::vector<std::string> extensions;

        struct Dimensions {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

        void destroy(VkDevice device);
        void loadFromFile(std::string filename, VulkanDevice* device, VkQueue transferQueue, float scale = 1.0f);

    void getSceneDimensions();


};


#endif //AR_ENGINE_VKMYMODEL_H

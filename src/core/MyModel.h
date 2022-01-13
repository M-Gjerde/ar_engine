//
// Created by magnus on 10/5/21.
//

#ifndef AR_ENGINE_MYMODEL_H
#define AR_ENGINE_MYMODEL_H


#include <glm/glm.hpp>
#include <ar_engine/src/tools/Macros.h>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>
#include <ar_engine/src/tools/Utils.h>
#include <ar_engine/src/Renderer/shaderParams.h>
#include "VulkanDevice.h"
#include "Texture.h"

class MyModel {

public:
    MyModel();
    struct Model {
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec4 joint0;
            glm::vec4 weight0;
        };

        struct Vertices {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory;
        } vertices;
        struct Indices {
            int count;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory;
        } indices;

        std::vector<std::string> extensions;

        struct Dimensions {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

    } model;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
    } dimensions;


    struct UniformBufferSet {
        Buffer object;
        Buffer lightParams;
    };

    std::vector<UniformBufferSet> uniformBuffers{};

    std::vector<VkDescriptorSet> descriptors;
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};

    glm::mat4 aabb{};
    std::vector<Texture> textures;
    std::vector<std::string> extensions;


    void destroy(VkDevice device);

    void loadFromFile(std::string filename, float scale = 1.0f);

    void createDescriptorSetLayout();

    void createPipeline(VkRenderPass pT, std::vector<VkPipelineShaderStageCreateInfo> vector);

    void createPipelineLayout();

    void draw(VkCommandBuffer commandBuffer, uint32_t i);

    void updateUniformBufferData(uint32_t index, void* params, void* matrix);

    void prepareUniformBuffers(uint32_t count);

    void createDescriptors(uint32_t count);

protected:
    MyModel *self = this;
    VulkanDevice *vulkanDevice;

    void useStagingBuffer(Model::Vertex *_vertices, uint32_t vertexCount, unsigned int *_indices, uint32_t indexCount);

};


#endif //AR_ENGINE_MYMODEL_H

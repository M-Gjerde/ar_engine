//
// Created by magnus on 12/10/21.
//

#ifndef AR_ENGINE_GLTFMODEL_H
#define AR_ENGINE_GLTFMODEL_H


#include <vulkan/vulkan_core.h>
#include <glm/detail/type_quat.hpp>
#include "VulkanDevice.h"
#include "glm/glm.hpp"
#include "tiny_gltf.h"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

class glTFModel {
public:

    VulkanDevice* device;

    glTFModel();

    struct Primitive {
        uint32_t firstIndex{};
        uint32_t indexCount{};
        uint32_t vertexCount{};
        bool hasIndices{};
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount);
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };



    struct Mesh {
        VulkanDevice *device;
        std::vector<Primitive*> primitives;
        struct UniformBuffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDescriptorBufferInfo descriptor;
            VkDescriptorSet descriptorSet;
            void *mapped;
        } uniformBuffer{};
        struct UniformBlock {
            glm::mat4 matrix;
            glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
            float jointcount{ 0 };
        } uniformBlock;
        Mesh(VulkanDevice* device, glm::mat4 matrix);
        ~Mesh();
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    struct Node {
        Node *parent;
        uint32_t index;
        std::vector<Node*> children;
        glm::mat4 matrix;
        std::string name;
        Mesh *mesh;
        int32_t skinIndex = -1;
        glm::vec3 translation{};
        glm::vec3 scale{ 1.0f };
        glm::quat rotation{};
        glm::mat4 localMatrix();
        glm::mat4 getMatrix();
        void update();
        ~Node();
    };

    struct Skin {
        std::string name;
        Node *skeletonRoot = nullptr;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Node*> joints;
    };




    struct Model {

        VulkanDevice *device;
        std::vector<Skin*> skins;
        std::vector<std::string> extensions;

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


        std::vector<Node*> nodes;
        std::vector<Node*> linearNodes;

        struct Dimensions {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

        void destroy(VkDevice device);
        void loadNode(glTFModel::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale);
        void loadSkins(tinygltf::Model& gltfModel);
        void loadTextures(tinygltf::Model& gltfModel, VulkanDevice* device, VkQueue transferQueue);
        VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
        VkFilter getVkFilterMode(int32_t filterMode);
        void loadTextureSamplers(tinygltf::Model& gltfModel);
        void loadMaterials(tinygltf::Model& gltfModel);
        void loadAnimations(tinygltf::Model& gltfModel);
        void loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue, float scale);
        void drawNode(Node* node, VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);
        void calculateBoundingBox(Node* node, Node* parent);
        void getSceneDimensions();
        void updateAnimation(uint32_t index, float time);
        Node* findNode(Node* parent, uint32_t index);
        Node* nodeFromIndex(uint32_t index);
    } model;

    struct UniformBufferSet {
        Buffer model;
        Buffer shaderValues;
    };

    std::vector<UniformBufferSet> uniformBuffers{};
    std::vector<VkDescriptorSet> descriptors;
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayout descriptorSetLayoutNode{};

    VkDescriptorPool descriptorPool{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};

    void prepareUniformBuffers(uint32_t count);


    void updateUniformBufferData(uint32_t index, void* params, void* matrix);

    void createDescriptorSetLayout();

    void createDescriptors(uint32_t count);

    void setupNodeDescriptorSet(Node *node);

    void createPipeline(VkRenderPass renderPass, std::vector<VkPipelineShaderStageCreateInfo> shaderStages);


    void draw(VkCommandBuffer commandBuffer, uint32_t i);
    void drawNode(Node *node, VkCommandBuffer commandBuffer);
};


#endif //AR_ENGINE_GLTFMODEL_H

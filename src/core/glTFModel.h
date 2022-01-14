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
#include "Texture.h"

#include <glm/gtc/type_ptr.hpp>
#include <ar_engine/src/tools/Macros.h>
#include <ar_engine/src/Renderer/shaderParams.h>
#include "Base.h"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

class glTFModel {
public:

    glTFModel();

    VulkanDevice *vulkanDevice;

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

    } mesh;

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

    struct Material {
        enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(1.0f);
        Texture2D *baseColorTexture;
        Texture2D *normalTexture;
        struct TexCoordSets {
            uint8_t baseColor = 0;
            uint8_t metallicRoughness = 0;
            uint8_t specularGlossiness = 0;
            uint8_t normal = 0;
            uint8_t occlusion = 0;
            uint8_t emissive = 0;
        } texCoordSets;
        struct PbrWorkflows {
            bool metallicRoughness = true;
            bool specularGlossiness = false;
        } pbrWorkflows;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    struct TextureIndices{
        uint32_t baseColor = -1;
        uint32_t normalMap = -1;
    } ;

    struct Model {

        VulkanDevice *device;
        std::vector<Skin*> skins;
        std::vector<std::string> extensions;
        std::vector<Primitive> primitives;
        std::vector<Texture2D> textures;
        std::vector<Material> materials;
        std::vector<Texture::TextureSampler> textureSamplers;
        TextureIndices textureIndices;

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
        void loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue, float scale);
        Node* findNode(Node* parent, uint32_t index);
        Node* nodeFromIndex(uint32_t index);
    } model;


    std::vector<VkDescriptorSet> descriptors;
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayout descriptorSetLayoutNode{};

    VkDescriptorPool descriptorPool{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};


    void createDescriptorSetLayout();

    void createDescriptors(uint32_t count, std::vector<Base::UniformBufferSet> ubo);

    void setupNodeDescriptorSet(Node *node);

    void createPipeline(VkRenderPass renderPass, std::vector<VkPipelineShaderStageCreateInfo> shaderStages);


    void draw(VkCommandBuffer commandBuffer, uint32_t i);
    void drawNode(Node *node, VkCommandBuffer commandBuffer);
};


#endif //AR_ENGINE_GLTFMODEL_H

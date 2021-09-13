//
// Created by magnus on 9/13/21.
//
#ifndef AR_ENGINE_GLTFMODEL_H
#define AR_ENGINE_GLTFMODEL_H

#include <ar_engine/src/core/VulkanDevice.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ar_engine/src/core/Texture.h>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

class glTFModel {
public:
    // Single vertex buffer for all primitives
    struct {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertices;

    // Single index buffer for all primitives
    struct {
        int count;
        VkBuffer buffer;
        VkDeviceMemory memory;
    } indices;

    // The following structures roughly represent the glTF scene structure
    // To keep things simple, they only contain those properties that are required for this sample
    struct Node;

    // A primitive contains the data for a single draw call
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;
    };

    // Contains the node's (optional) geometry and can be made up of an arbitrary number of primitives
    struct Mesh {
        std::vector<Primitive> primitives;
    };

    // A node represents an object in the glTF scene graph
    struct Node {
        Node *parent;
        std::vector<Node> children;
        Mesh mesh;
        glm::mat4 matrix;
    };

    // A glTF material stores information in e.g. the texture that is attached to it and colors
    struct Material {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        uint32_t baseColorTextureIndex;
    };

    // Contains the texture for a single glTF image
    // Images may be reused by texture objects and are as such separated
    struct Image {
        Texture2D texture;
        // We also store (and create) a descriptor set that's used to access this texture from the fragment shader
        VkDescriptorSet descriptorSet;
    };

    // A glTF texture stores a reference to the image and a sampler
    // In this sample, we are only interested in the image
    struct Texture {
        int32_t imageIndex;
    };

// The vertex layout for the samples' model
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 color;
    };

    /*
		Model data
	*/
    std::vector<Image> images;
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<Node> nodes;


    void loadNode(const tinygltf::Node &inputNode, const tinygltf::Model &input, Node *parent,
                  std::vector<uint32_t> &indexBuffer, std::vector<glTFModel::Vertex> &vertexBuffer);

    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node node);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

// The class requires some Vulkan objects so it can create it's own resources
    VulkanDevice *vulkanDevice;
    VkQueue copyQueue;

    void loadMaterials(tinygltf::Model &input);

    void loadTextures(tinygltf::Model &input);

    void loadImages(tinygltf::Model &input);


};


#endif //AR_ENGINE_GLTFMODEL_H

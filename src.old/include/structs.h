//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_STRUCTS_H
#define AR_ENGINE_STRUCTS_H


#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vector>
#include "triangle.h"
#include <string>
#include <array>

struct ArMeshInfoUI {
    std::string name;
    float floatVal = 0.0f;
    float minRange;
    float maxRange;
    bool active = false;
    std::string type;
    int intVal;
    bool update;
};

struct ThreadPushConstantBlock {
    glm::mat4 mvp;
    glm::vec3 color;
};


// UI params are set via push constants
struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};


// Options and values to display/toggle from the UI
struct UISettings {
    bool displayModels = true;
    bool displayLogos = true;
    bool displayBackground = true;
    bool animateLight = false;
    float lightSpeed = 0.25f;
    float average = 0.0f;
    float frameLimiter = 0.0f;
    float FPS = 0.0f;
    std::array<float, 1000> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
    float lightTimer = 0.0f;
};

struct ArROI {
    int x;
    int y;
    int width;
    int height;
    bool active = false;
};

struct ArShadersPath {
    std::string vertexShader;
    std::string fragmentShader;
    std::string computeShader;
};

// ArModel passes queue and cmd pool from arEngine and used to store buffers and memory
struct ArModel {
    VkQueue transferQueue{};              // TODO out from here. Use handles in arEngine
    VkCommandPool transferCommandPool{};  // TODO out from here. Use handles in arEngine
    std::vector<uint32_t> indices{};
    std::vector<Vertex> vertices;
    uint32_t vertexCount{};
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};
    uint32_t indexCount{};
    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};
};

// ArModelInfo stores general information that is used upon creation
struct ArModelInfo {

    std::string modelName;
    bool generateNormals = true;
};



struct ArDescriptorInfo {
    const uint32_t *pBindings;
    uint32_t descriptorPoolCount;
    VkDescriptorType *pDescriptorType;  // Array of different types 1:1 relationship
    uint32_t descriptorCount;           // Should be equal to all the numbers of pDescriptorSplitCount array summed
    uint32_t *pDescriptorSplitCount;
    VkShaderStageFlags* stageFlags;      // stageFlags for descriptors
    uint32_t descriptorSetLayoutCount;  // How many descriptor layouts
    uint32_t descriptorSetCount;
    uint32_t *dataSizes;
    VkSampler sampler;
    VkImageView view;
};


struct ArTextureImage {
    VkImageView textureImageView;
    VkImage textureImage;
    VkSampler textureSampler;
    VkDeviceMemory textureImageMemory;
    VkCommandPool transferCommandPool;
    VkQueue transferQueue;
    int width;
    int height;
    int channels;
    void *data;
};

struct ArBuffer {
    VkDeviceSize bufferSize{};
    VkBufferUsageFlags bufferUsage{};
    VkMemoryPropertyFlags bufferProperties{};
    VkBuffer buffer{};
    VkDeviceMemory bufferMemory{};
    void* mapped = nullptr;
    VkSharingMode sharingMode{};
    uint32_t queueFamilyIndexCount{};
    const uint32_t *pQueueFamilyIndices{};
};


struct MainDevice {
    VkDevice device;
    VkPhysicalDevice physicalDevice;

};

struct ArEngine {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    MainDevice mainDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    VkCommandPool commandPool;
};

struct ArPipeline {
    VkDevice device;
    VkFormat swapchainImageFormat;
    VkFormat depthFormat;
    VkExtent2D swapchainExtent;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

struct ArDepthResource {
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkExtent2D swapChainExtent;
};

struct ArDescriptor {
    VkDescriptorPool pDescriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<ArBuffer> bufferObject;
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
    uint32_t descriptorSetLayoutCount;
    VkDescriptorSetLayout *pDescriptorSetLayouts;
    std::vector<uint32_t> dataSizes;
};


struct ArCompute {
    VkCommandBuffer commandBuffer;
    ArDescriptor descriptor;
};

struct ArSharedMemory {
    unsigned char imgOne[1843300];
    size_t imgLen1;
    unsigned char imgTwo[1843300];
    size_t imgLen2;
};

struct Status {
    bool isRunning = false;
};

struct ArSceneObjectInfo {
    //Descriptors descriptor;
};


#endif //AR_ENGINE_STRUCTS_H
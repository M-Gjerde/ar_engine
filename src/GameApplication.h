//
// Created by magnus on 9/4/21.
//

#ifndef AR_ENGINE_GAMEAPPLICATION_H
#define AR_ENGINE_GAMEAPPLICATION_H


#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#define GLFW_INCLUDE_VULKAN

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui_impl_glfw.h>
#include <thread>
#include <ar_engine/src/imgui/ImGUI.h>
#include <ar_engine/src/core/ScriptBuilder.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ar_engine/src/core/VulkanRenderer.h"

class GameApplication : VulkanRenderer {

public:

    ImGUI* imgui;
    std::vector<std::unique_ptr<Base>> scripts;
    std::vector<SceneObject> sceneObjects;

    explicit GameApplication(const std::string &title) : VulkanRenderer(true) {
        // During constructor prepare backend for rendering
        VulkanRenderer::prepare();
        backendInitialized = true;

    };

    void prepareEngine();

    void run() {
        renderLoop();

    }


    ~GameApplication() override = default;

    void render() override;
    void draw();

private:

    // Vertex buffer and attributes
    struct {
        VkDeviceMemory memory; // Handle to the device memory for this buffer
        VkBuffer buffer;       // Handle to the Vulkan buffer object that the memory is bound to
    } vertices{};

    // Index buffer
    struct {
        VkDeviceMemory memory;
        VkBuffer buffer;
        uint32_t count;
    } indices{};

    // Uniform buffer block object
    struct {
        VkDeviceMemory memory;
        VkBuffer buffer;
        VkDescriptorBufferInfo descriptor;
    } uniformBufferVS{};

    // For simplicity we use the same uniform block layout as in the shader:
    //
    //	layout(set = 0, binding = 0) uniform UBO
    //	{
    //		mat4 projectionMatrix;
    //		mat4 modelMatrix;
    //		mat4 viewMatrix;
    //	} ubo;
    //
    // This way we can just memcopy the ubo data to the ubo
    // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
    struct {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } uboVS{};

    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    VkDescriptorSet descriptorSet{};
    VkDescriptorSetLayout descriptorSetLayout{};

    void addDeviceFeatures() override;

    void buildCommandBuffers() override;

    void setupDescriptorPool();

    void setupDescriptorSetLayout();

    void setupDescriptorSet();

    void preparePipelines();

    VkShaderModule loadSPIRVShader(std::string filename);

    void updateUniformBuffers();

    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

    void prepareVertices();

    void prepare() override;

    void prepareUniformBuffers();

    void viewChanged() override;

    void generateScriptClasses();
};


#endif //AR_ENGINE_GAMEAPPLICATION_H

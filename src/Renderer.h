//
// Created by magnus on 9/4/21.
//

#ifndef AR_ENGINE_RENDERER_H
#define AR_ENGINE_RENDERER_H



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

#include "ar_engine/src/discarded/glTFModel.h"

class Renderer : VulkanRenderer {

public:

    ImGUI* imgui;
    std::vector<std::unique_ptr<Base>> scripts;
    std::vector<SceneObject> sceneObjects;

    explicit Renderer(const std::string &title) : VulkanRenderer(true) {
        // During constructor prepare backend for rendering
        VulkanRenderer::prepare();
        backendInitialized = true;
    };

    void prepareEngine();

    void run() {
        renderLoop();

    }


    ~Renderer() override = default;

    void render() override;
    void draw();

private:

    bool wireframe = false;


    struct ShaderData {
        Buffer buffer;
        struct Values {
            glm::mat4 projection;
            glm::mat4 model;
            glm::vec4 lightPos = glm::vec4(5.0f, 5.0f, -5.0f, 1.0f);
        } values;
    } shaderData;

    struct Pipelines {
        VkPipeline solid;
        VkPipeline wireframe = VK_NULL_HANDLE;
    } pipelines;

    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout matrices;
        VkDescriptorSetLayout textures;
    } descriptorSetLayouts;



    void setupDescriptors();
    void preparePipelines();

    VkShaderModule loadSPIRVShader(std::string filename);

    void updateUniformBuffers();
    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
    void prepareVertices();
    void prepareUniformBuffers();

    void viewChanged() override;
    void addDeviceFeatures() override;
    void buildCommandBuffers() override;
    void prepare() override;

    void generateScriptClasses();
    void loadglTFFile(std::string filename);
    void loadAssets();
};


#endif //AR_ENGINE_RENDERER_H

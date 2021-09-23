//
// Created by magnus on 9/4/21.
//

#ifndef AR_ENGINE_RENDERER_H
#define AR_ENGINE_RENDERER_H


// System
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>

// External
#include <tiny_obj_loader.h>

// Engine
#include "ar_engine/src/builder/Example.h"
#include <ar_engine/src/builder/Terrain.h>
#include <ar_engine/src/tools/Macros.h>
#include <ar_engine/src/core/ScriptBuilder.h>
#include <ar_engine/src/core/VulkanglTFModel.h>
#include <ar_engine/src/core/Texture.h>
#include "ar_engine/src/core/VulkanRenderer.h"

class Renderer : VulkanRenderer {

public:

    std::vector<std::unique_ptr<Base>> scripts;
    std::vector<SceneObject> sceneObjects;

    explicit Renderer(const std::string &title) : VulkanRenderer(true) {
        // During constructor prepare backend for rendering
        VulkanRenderer::prepare();
        backendInitialized = true;

        prepareRenderer();
    };
     ~Renderer() override = default;

    void render() override;
    void prepareRenderer();
    void run() {
        renderLoop();
    }

    void draw();

private:


protected:

    bool wireframe = false;

    struct Textures {
        TextureCubeMap environmentCube;
        Texture2D empty;
        Texture2D lutBrdf;
        TextureCubeMap irradianceCube;
        TextureCubeMap prefilteredCube;
    } textures{};

    struct ShaderData {
        Buffer buffer;
        struct Values {
            glm::mat4 projection;
            glm::mat4 model;
            glm::vec4 lightPos = glm::vec4(5.0f, 5.0f, -5.0f, 1.0f);
        } values;
    } shaderData;

    struct LightSource {
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
    } lightSource;



    struct shaderValuesParams {
        glm::vec4 lightDir;
        float exposure = 4.5f;
        float gamma = 2.2f;
        float prefilteredCubeMipLevels;
        float scaleIBLAmbient = 1.0f;
        float debugViewInputs = 0;
        float debugViewEquation = 0;
    } shaderValuesParams;

    struct Models {
        vkglTF::Model scene;
        vkglTF::Model skybox;
    } models;

    struct UniformBufferSet {
        Buffer scene;
        Buffer skybox;
        Buffer params;
    };

    struct UBOMatrices {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        glm::vec3 camPos;
    } shaderValuesScene{}, shaderValuesSkybox{};

    VkPipelineLayout pipelineLayout{};

    struct Pipelines {
        VkPipeline skybox;
        VkPipeline pbr;
        VkPipeline pbrAlphaBlend;
    } pipelines{};

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout material;
        VkDescriptorSetLayout node;
    } descriptorSetLayouts{};

    struct DescriptorSets {
        VkDescriptorSet scene;
        VkDescriptorSet skybox;
    };

    struct PushConstBlockMaterial {
        glm::vec4 baseColorFactor;
        glm::vec4 emissiveFactor;
        glm::vec4 diffuseFactor;
        glm::vec4 specularFactor;
        float workflow;
        int colorTextureSet;
        int PhysicalDescriptorTextureSet;
        int normalTextureSet;
        int occlusionTextureSet;
        int emissiveTextureSet;
        float metallicFactor;
        float roughnessFactor;
        float alphaMask;
        float alphaMaskCutoff;
    } pushConstBlockMaterial{};

    void setupDescriptors();
    void preparePipelines();

    void updateUniformBuffers();
    void prepareUniformBuffers();

    void viewChanged() override;
    void addDeviceFeatures() override;
    void buildCommandBuffers() override;
    void prepare() override;
    void UIUpdate(UISettings uiSettings) override;

    void generateScriptClasses();
    void loadglTFFile(std::string filename);
    void loadAssets();

    void loadCube();
};


#endif //AR_ENGINE_RENDERER_H

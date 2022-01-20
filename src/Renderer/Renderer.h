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

// Engine
#include "ar_engine/src/builder/Example.h"
#include <ar_engine/src/builder/Terrain.h>
#include <ar_engine/src/builder/DamagedHelmet.h>
#include <ar_engine/src/builder/Sun.h>
#include <ar_engine/src/builder/Mercury.h>

#include <ar_engine/src/tools/Macros.h>

#include <ar_engine/src/core/VulkanglTFModel.h>
#include <ar_engine/src/core/Texture.h>
#include <ar_engine/src/core/MyModel.h>
#include <ar_engine/src/core/VulkanRenderer.h>
#include <ar_engine/src/Renderer/shaderParams.h>


class Renderer : VulkanRenderer {

public:

    std::vector<std::unique_ptr<Base>> scripts;

    explicit Renderer(const std::string &title) : VulkanRenderer(true) {
        // During constructor prepare backend for rendering
        VulkanRenderer::initVulkan();
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

    enum PBRWorkflows{ PBR_WORKFLOW_METALLIC_ROUGHNESS = 0, PBR_WORKFLOW_SPECULAR_GLOSINESS = 1 };

    struct Textures {
        TextureCubeMap environmentCube;
        Texture2D empty;
        Texture2D lutBrdf;
        TextureCubeMap irradianceCube;
        TextureCubeMap prefilteredCube;
    } textures{};


    UBOMatrix *UBOVert{};
    UBOMatrix * UBOskybox{};
    FragShaderParams *UBOFrag{};
    //ShaderValuesParams *shaderValuesPasrams{};

    struct Models {
        vkglTF::Model scene;
        vkglTF::Model skybox;
    } models;

    struct UniformBufferSet {
        Buffer skybox;
    };
    std::vector<UniformBufferSet> uniformBuffers{};

    VkPipelineLayout pipelineLayout{};

    struct Pipelines {
        VkPipeline skybox;
    } pipelines{};

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout skybox;
    } descriptorSetLayouts{};

    struct DescriptorSets {
        VkDescriptorSet skybox;
    };
    std::vector<DescriptorSets> descriptorSets;

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
    void loadAssets();


    void setupNodeDescriptorSet(vkglTF::Node *node);

    void generateBRDFLUT();

    void generateCubemaps();

    void renderNode(vkglTF::Node *node, uint32_t cbIndex, vkglTF::Material::AlphaMode alphaMode);

    void defaultUniformBuffers();

    void createSkybox();
};


#endif //AR_ENGINE_RENDERER_H

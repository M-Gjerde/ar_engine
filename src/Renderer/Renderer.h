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

#include <ar_engine/src/discarded/VulkanglTFModel.h>
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

    UBOMatrix *UBOVert{};
    FragShaderParams *UBOFrag{};


    void updateUniformBuffers();
    void prepareUniformBuffers();

    void viewChanged() override;
    void addDeviceFeatures() override;
    void buildCommandBuffers() override;
    void UIUpdate(UISettings uiSettings) override;

    void generateScriptClasses();

    void storeDepthFrame();
};


#endif //AR_ENGINE_RENDERER_H

//
// Created by magnus on 9/21/20.
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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "VulkanRenderer.h"

class GameApplication : VulkanRenderer {

public:

    explicit GameApplication(const std::string &title) : VulkanRenderer(true) {


        //glfwSetErrorCallback(error_callback);
        VulkanRenderer::prepare();
        prepared = true;


        // Create Engine Instance
        renderLoop();
    }

    void render() override {

        VulkanRenderer::prepareFrame();

        // Use a fence to wait until the command buffer has finished execution before using it again

        VkResult result = vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to wait for fence");
        result = vkResetFences(device, 1, &waitFences[currentBuffer]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to reset fence");


        submitInfo.commandBufferCount = 0;
        submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

        vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]);

        VulkanRenderer::submitFrame();

    }

    void gameLoop() {


    }


    void cursorPosCallback(GLFWwindow *newWindow, double xPos, double yPos) {};

    void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {};


private:

    static void error_callback(int error, const char *description) {
        fprintf(stderr, "Error: %s, code: %d\n", description, error);
    }

};


#endif //AR_ENGINE_GAMEAPPLICATION_H

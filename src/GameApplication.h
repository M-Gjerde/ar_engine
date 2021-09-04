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
        // Create Engine Instance

    prepared = true;
    }

    void render() override {

    }

    void keyCallback(GLFWwindow *glfWwindow, int key, int scancode, int action, int mods) const {
        printf("GLFW Key: %d\n", key);

        if (key == GLFW_KEY_ESCAPE and action == GLFW_PRESS){
            glfwSetWindowShouldClose(window, true);
        }

    };

    void cursorPosCallback(GLFWwindow *newWindow, double xPos, double yPos) {};
    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){};
    // must be overridden in derived class
    void update(){};

    void gameLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

        }
        glfwDestroyWindow(window);
        glfwTerminate();

    }

private:

    static void error_callback(int error, const char *description) {
        fprintf(stderr, "Error: %s, code: %d\n", description, error);
    }

};




#endif //AR_ENGINE_GAMEAPPLICATION_H

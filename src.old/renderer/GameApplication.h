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
#include "VulkanRenderer.hpp"
#include "../Platform/Camera.h"
#include "LoadSettings.h"
#include "../FaceAugment/ThreadSpawner.h"
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class GameApplication {

public:
    GLFWwindow *window;
    VulkanRenderer vulkanRenderer;
    std::vector<Camera*> cameras;
    LoadSettings *loadSettings;
    UISettings uiSettings;


    explicit GameApplication(const std::string &title) {

        // boilerplate stuff (ie. basic window setup, initialize OpenGL) occurs in abstract class
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(viewport.WIDTH, viewport.HEIGHT, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);

        glfwSetErrorCallback(error_callback);

        // Load settings
        loadSettings = new LoadSettings("config");

        // Initialize camera
        cameras.resize(2);
        cameras[0] = new Camera;
        vulkanRenderer.updateCamera(cameras[0]->getView(), cameras[0]->getProjection());

        // Init vulkan renderer engine
        if (vulkanRenderer.init(window) == EXIT_FAILURE)
            throw std::runtime_error("Failed to init");

        // Load scene objects according to settings file
        auto settingsMap = loadSettings->getSceneObjects();
        vulkanRenderer.setupSceneFromFile(settingsMap);

    }

    virtual void keyCallback(GLFWwindow *glfWwindow, int key, int scancode, int action, int mods) {};

    virtual void cursorPosCallback(GLFWwindow *newWindow, double xPos, double yPos) {};
    virtual void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){};
    // must be overridden in derived class
    virtual void update(){};

private:

    static void error_callback(int error, const char *description) {
        fprintf(stderr, "Error: %s\n", description);
    }

};



class AppExtension : public GameApplication {
public:

    explicit AppExtension(const std::string &title) : GameApplication(title) {

    }

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) override;
    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) override;
    void cursorPosCallback(GLFWwindow *window, double xPos, double yPos) override;

    void update() override;

private:
    double deltaTime = 0.0f;
    double lastTime = 0.0f;
    double FPS = 0.0f;

};






#endif //AR_ENGINE_GAMEAPPLICATION_H
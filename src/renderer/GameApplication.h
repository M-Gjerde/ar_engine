//
// Created by magnus on 9/21/20.
//


#ifndef AR_ENGINE_GAMEAPPLICATION_H
#define AR_ENGINE_GAMEAPPLICATION_H


#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include "VulkanRenderer.hpp"
#include "../include/Camera.h"


class GameApplication {

public:
    GLFWwindow *window;
    VulkanRenderer vulkanRenderer;
    Camera camera;

    explicit GameApplication(const std::string &title) {

        // boilerplate stuff (ie. basic window setup, initialize OpenGL) occurs in abstract class
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(viewport.WIDTH, viewport.HEIGHT, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);

        glfwSetErrorCallback(error_callback);

        if (vulkanRenderer.init(window) == EXIT_FAILURE)
            throw std::runtime_error("Failed to init");


        vulkanRenderer.updateCamera(camera.getView(), camera.getProjection());

    }

    virtual void keyCallback(GLFWwindow *glfWwindow, int key, int scancode, int action, int mods) {};

    virtual void cursorPosCallback(GLFWwindow *newWindow, double xPos, double yPos) {};

    // must be overridden in derived class
    virtual void update(){};

    void gameLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            update();
            vulkanRenderer.draw();
        }

        vulkanRenderer.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

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

    void cursorPosCallback(GLFWwindow *window, double xPos, double yPos) override;

    void update() override;

private:
    double deltaTime = 0.0f;
    double lastTime = 0.0f;

};

GameApplication *getApplication() {
    return new AppExtension("AppExtension");
}




#endif //AR_ENGINE_GAMEAPPLICATION_H

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
#include "../headers/VulkanRenderer.hpp"


class GameApplication {

public:
    GLFWwindow *window;
    VulkanRenderer vulkanRenderer;
    explicit GameApplication(std::string title) {

        // boilerplate stuff (ie. basic window setup, initialize OpenGL) occurs in abstract class
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(640, 480, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);

        glfwSetErrorCallback(error_callback);

        // my application specific state gets initialized here
        if (vulkanRenderer.init(window) == EXIT_FAILURE)
            throw std::runtime_error("Failed to init");

    }

    virtual void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        // basic window handling
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {

            std::cout << "exiting..." << std::endl;
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }
    // must be overridden in derived class
    virtual void update() = 0;

    void gameLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            update();

        }
        vulkanRenderer.cleanup();
        glfwTerminate();
    }

private:
    static void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error: %s\n", description);
    }

};


class AppExtension : public GameApplication {
public:

    explicit AppExtension(std::string title) : GameApplication(title) {

    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) override {
        GameApplication::keyCallback(window, key, scancode, action, mods);

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            // anotherClass.Draw();
            std::cout << "space pressed!" << std::endl;
        }
    }

    int helicopter = vulkanRenderer.createMeshModel("../objects/uh60.obj");
    double angle = 0.0f;
    double deltaTime = 0.0f;
    double lastTime = 0.0f;

    void update() override {
        double now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 0.1f * deltaTime;
        if (angle > 10) angle -= 10.0f;

        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(0)), glm::vec3(0.0f, 1.0f, 0.0f));;
        vulkanRenderer.updateModel(helicopter, testMat);
        vulkanRenderer.draw();
    }
};

GameApplication* getApplication() {
    return new AppExtension("AppExtension");
}




#endif //AR_ENGINE_GAMEAPPLICATION_H

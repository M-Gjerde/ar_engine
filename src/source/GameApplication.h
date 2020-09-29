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
#include "../headers/Camera.h"


class GameApplication {

public:
    GLFWwindow *window;
    VulkanRenderer vulkanRenderer;
    Camera camera;

    void initCamera() {

        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float) 1280 /
                                                (float) 480, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(5.0f, 0.0f, 20.0f),
                                     glm::vec3(0.0f, 0.0f, -5.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        projection[1][1] *= -1; // Flip the y-axis because Vulkan and OpenGL are different and glm was initially made for openGL

        camera.setProjection(projection);
        camera.setView(view);
        camera.enableCamera(true);

    }

    explicit GameApplication(const std::string& title) {

        // boilerplate stuff (ie. basic window setup, initialize OpenGL) occurs in abstract class
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(640 * 2, 480, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);

        glfwSetErrorCallback(error_callback);

        // my application specific state gets initialized here
        if (vulkanRenderer.init(window) == EXIT_FAILURE)
            throw std::runtime_error("Failed to init");

        // Setup Scene Camera
        initCamera();
        vulkanRenderer.setCamera(camera);

    }

    virtual void keyCallback(GLFWwindow *glfWwindow, int key, int scancode, int action, int mods) {
        // basic glfWwindow handling
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {

            std::cout << "exiting..." << std::endl;
            glfwSetWindowShouldClose(glfWwindow, GL_TRUE);
        }
    }

    virtual void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {

    }

    // must be overridden in derived class
    virtual void update() = 0;

    void gameLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            update();

            vulkanRenderer.draw();
        }
        vulkanRenderer.cleanup();
        glfwTerminate();
    }

private:

    static void error_callback(int error, const char *description) {
        fprintf(stderr, "Error: %s\n", description);
    }

};


class AppExtension : public GameApplication {
public:

    explicit AppExtension(const std::string& title) : GameApplication(title) {

    }


    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) override;
    void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

    void update() override;

private:
    // Scene settings
    int helicopter = 0;//vulkanRenderer.createMeshModel("../objects/uh60.obj");

    double motion = 0.0f;
    [[maybe_unused]] double deltaTime = 0.0f;
    double lastTime = 0.0f;

};

GameApplication *getApplication() {
    return new AppExtension("AppExtension");
}


#endif //AR_ENGINE_GAMEAPPLICATION_H

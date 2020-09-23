//
// Created by magnus on 9/21/20.
//

#include "GameApplication.h"



void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;


}

void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    GameApplication::keyCallback(window, key, scancode, action, mods);

    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {

        std::cout << "Up pressed!" << std::endl;
    }


    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        std::cout << "space pressed!" << std::endl;
    }


    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        std::cout << "Down pressed!" << std::endl;
    }


    if (key == GLFW_KEY_RIGHT) {
        std::cout << "Right pressed!" << std::endl;
        motion += 0.5f;
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(motion, 0, 0));
        vulkanRenderer.updateModel(helicopter, translate);

    }

    if (key == GLFW_KEY_LEFT) {
        std::cout << "Left pressed!" << std::endl;
        motion -= 0.5f;

        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(motion, 0, 0));
        vulkanRenderer.updateModel(helicopter, translate);

    }
}

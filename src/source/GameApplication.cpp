//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include "GameApplication.h"


double angle = 0;
void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;
    angle += 10.0f * deltaTime;
    if (angle > 360.0f) { angle -= 360.0f; }

    //glm::mat4 testMat = glm::mat4(0.0f);


    glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(angle)), glm::vec3(0.0f, 0.0f, 1.0f));
    vulkanRenderer.updateTriangle(testMat);


}

int motion2 = 0;

void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    GameApplication::keyCallback(window, key, scancode, action, mods);

    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {

        std::cout << "Up pressed!" << std::endl;
    }


    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        std::cout << "space pressed!" << std::endl;
        motion2 += 10;
        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(motion2)), glm::vec3(0.0f, 0.0f, 1.0f));
        vulkanRenderer.updateTriangle(testMat);

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

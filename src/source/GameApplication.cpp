//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include "GameApplication.h"

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 10;


void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;

    // Keep update function at 24 frames per second
    if (deltaTime < 0.03) {
        double timeToSleep = (0.03 - deltaTime) * 1000;
        usleep(timeToSleep);
    }
}


void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    GameApplication::keyCallback(window, key, scancode, action, mods);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {

    }
    if (key == GLFW_KEY_H) {
    }


    if (key == GLFW_KEY_UP) {
    }

    if (key == GLFW_KEY_DOWN) {

    }

    if (key == GLFW_KEY_RIGHT) {

    }

    if (key == GLFW_KEY_LEFT) {

    }

    if (key == GLFW_KEY_W)
        cameraPos += cameraSpeed * cameraFront;
    if (key == GLFW_KEY_S)
        cameraPos -= cameraSpeed * cameraFront;
    if (key == GLFW_KEY_A)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == GLFW_KEY_D)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;


}

void AppExtension::cursorPosCallback(GLFWwindow *window, double xPos, double yPos) {
    //printf("Cursor position: (%f, %f)\n", xPos, yPos);

}

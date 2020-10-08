//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include "GameApplication.h"


void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;

    // Keep update function at 24 frames per second
    if (deltaTime < 0.03) {
        double timeToSleep = (0.03 - deltaTime) * 1000;
        usleep(timeToSleep);
    }

    vulkanRenderer.updateModel(glm::mat4(1.0f));
}


void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        std::cout << "exiting..." << std::endl;
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_RIGHT)
        camera.rotateRight();
    if (key == GLFW_KEY_LEFT)
        camera.rotateLeft();
    if (key == GLFW_KEY_W)
        camera.forward();
    if (key == GLFW_KEY_S)
        camera.backward();
    if (key == GLFW_KEY_A)
        camera.moveLeft();
    if (key == GLFW_KEY_D)
        camera.moveRight();

    vulkanRenderer.updateCamera(camera.getView(), camera.getProjection());

}

void AppExtension::cursorPosCallback(GLFWwindow *window, double xPos, double yPos) {
   // printf("Cursor position: (%f, %f)\n", xPos, yPos);

}

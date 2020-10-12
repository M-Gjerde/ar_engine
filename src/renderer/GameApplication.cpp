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
        disparity.stopProgram();

        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        disparity.input = 3;
        std::cout << "disparity input: " << disparity.input << std::endl;

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

double xPos, yPos;

void AppExtension::cursorPosCallback(GLFWwindow *window, double _xPos, double _yPos) {
   xPos = _xPos;
   yPos = _yPos;

}

void AppExtension::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        printf("Mouse click at pos: %f, %f\n", xPos, yPos);

}


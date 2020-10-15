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

    if (key == GLFW_KEY_UP) {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
        trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0, 0, 1));
        vulkanRenderer.updateModel(trans, 1);
    }
    if (key == GLFW_KEY_DOWN) {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
        vulkanRenderer.updateModel(trans, 0);
    }

    if (key == GLFW_KEY_LEFT_SHIFT) {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
        vulkanRenderer.updateModel(trans, 0);
        trans = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
        vulkanRenderer.updateModel(trans, 1);

    }

    if (key == GLFW_KEY_LEFT_CONTROL) {
        vulkanRenderer.updateTexture("texture2");
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
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        printf("Mouse click at pos: %f, %f\n", xPos, yPos);

        printf("____________________________________________\n "
               "Camera data: yaw = %f\n", camera.yaw);

        printf("cameraFront (x,y,z): %f, %f, %f\n", camera.cameraFront.x, camera.cameraFront.y, camera.cameraFront.z);
    }

}


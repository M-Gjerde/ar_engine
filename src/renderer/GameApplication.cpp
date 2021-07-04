//
// Created by magnus on 9/21/20.
//

#include <thread>
#include "GameApplication.h"

void AppExtension::update() {
    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;

    // Keep update function at 24 frames per second
    if (deltaTime < 0.03) {
        auto timeToSleep = (unsigned int) ((0.03 - deltaTime) * 1000);
        usleep(timeToSleep);
    }


}

void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        std::cout << "exiting..." << std::endl;
        vulkanRenderer.stopDisparityStream();
        glfwSetWindowShouldClose(window, true);
    }

    // execute vulkan compute sequence
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        //vulkanRenderer.loadSphereObjects();
        //vulkanRenderer.updateScene();
        vulkanRenderer.startDisparityStream();

    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        vulkanRenderer.stopDisparityStream();

    }


    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        vulkanRenderer.testFunction();
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

void AppExtension::cursorPosCallback(GLFWwindow *window, double _xPos, double _yPos) {
    camera.lookAround(_xPos, _yPos);
    vulkanRenderer.updateCamera(camera.getView(), camera.getProjection());

}

bool hiddenCursor = false;

void AppExtension::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        if (hiddenCursor) {
            hiddenCursor = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); //Normal mouse mode input
        } else {
            hiddenCursor = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // FPS mode mouse input

        }

        printf("____________________________________________\n "
               "Camera data: yaw = %f\n", camera.yaw);

        printf("cameraFront (x,y,z): %f, %f, %f\n", camera.cameraFront.x, camera.cameraFront.y, camera.cameraFront.z);
    }

}


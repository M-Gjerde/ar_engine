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

        // Toggle texture renderer from disparity
        if (vulkanRenderer.textureUpdateToggle)
            vulkanRenderer.textureUpdateToggle = false;
        //vulkanRenderer.updateDisparityVideoTexture();
        vulkanRenderer.textureUpdateToggle = true;


    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        disparity.input = 2;
        std::cout << "disparity input: " << disparity.input << std::endl;

    }

    if (key == GLFW_KEY_UP) {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 1.0f));
        trans = glm::scale(trans, glm::vec3(10.0f, 2.50f, 1.0f));
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
        trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        vulkanRenderer.updateModel(trans, 2);


    }

    if (key == GLFW_KEY_LEFT_CONTROL  && action == GLFW_PRESS) {
        vulkanRenderer.updateTextureImage("stereo/image_01/data/0000000001.png");
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
    if (key == GLFW_KEY_E)
        camera.roll();

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


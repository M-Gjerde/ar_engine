//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include <thread>
#include "GameApplication.h"

bool rotate = false;
float angle = 0;
float rColor = 0;
glm::mat4 rotateMat;

glm::vec3 lightPos = glm::vec3(5.0f, 0.0f, 3.0f);
glm::mat4 lightTrans(1.0f);

void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;


    if (rotate) {
        angle += 50.0f * static_cast<float>(deltaTime);
        if (angle > 360) angle -= 360.0f;
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
        rotateMat = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        rotateMat = glm::rotate(rotateMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        vulkanRenderer.updateModel(rotateMat, 2, false);

    }


    /*
    // Keep update function at 24 frames per second
    if (deltaTime < 0.03) {
        double timeToSleep = (0.03 - deltaTime) * 1000;
        usleep(timeToSleep);
    }
*/

    //vulkanRenderer.updateSpecularLightCamera(camera.cameraPos);
}


float forward = 5, right = 0, rotation = 0;
float model_angle = 0;
int modelIndex = 1;

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

bool hiddenCursor = true;

void AppExtension::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        if (hiddenCursor) {
            hiddenCursor = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); //FPS mode mouse input
        } else {
            hiddenCursor = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //Normal mouse mode input

        }

        printf("____________________________________________\n "
               "Camera data: yaw = %f\n", camera.yaw);

        printf("cameraFront (x,y,z): %f, %f, %f\n", camera.cameraFront.x, camera.cameraFront.y, camera.cameraFront.z);
    }

}


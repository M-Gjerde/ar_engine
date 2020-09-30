//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include "GameApplication.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

float zMovement = 0;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float cameraSpeed = 1;

float yaw = 0;
double angle = 0;


void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;
    angle += 10.0f * deltaTime;
    if (angle > 360.0f) { angle -= 360.0f; }
    //glm::mat4 testMat = glm::mat4(0.0f);




    // Keep update function at 24 frames per second
    if (deltaTime < 0.03){
        double timeToSleep = (0.03 - deltaTime) * 1000;
        usleep(timeToSleep);
    }

}


glm::mat4 trianglePos;

void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    GameApplication::keyCallback(window, key, scancode, action, mods);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {

        trianglePos = vulkanRenderer.getTrianglePosition();
        std::cout << "space pressed!" << std::endl;

        printf("x: %f\n", trianglePos[3][0]);
        printf("y: %f\n", trianglePos[3][1]);
        printf("z: %f\n", trianglePos[3][2]);


    }
    if (key == GLFW_KEY_H){
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10));
        vulkanRenderer.updateModel(helicopter, trans);
    }


    if (key == GLFW_KEY_UP) {
        zMovement++;
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zMovement));

        printf("x: %f\n", trans[3][0]);
        printf("y: %f\n", trans[3][1]);
        printf("z: %f\n", trans[3][2]);
        vulkanRenderer.updateTriangle(trans);
    }



    if (key == GLFW_KEY_DOWN) {
        zMovement--;

        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zMovement));
        vulkanRenderer.updateTriangle(trans);

    }


    if (key == GLFW_KEY_RIGHT) {
        std::cout << "Right pressed!" << std::endl;

        yaw += 1;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw));
        direction.z = sin(glm::radians(yaw));
        cameraFront = glm::normalize(direction);

    }

    if (key == GLFW_KEY_LEFT) {
        std::cout << "Left pressed!" << std::endl;
        yaw -= 1;
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw));
        direction.z = sin(glm::radians(yaw));
        cameraFront = glm::normalize(direction);

    }

    if (key == GLFW_KEY_W)
        cameraPos += cameraSpeed * cameraFront;
    if (key == GLFW_KEY_S)
        cameraPos -= cameraSpeed * cameraFront;
    if (key == GLFW_KEY_A)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == GLFW_KEY_D)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    vulkanRenderer.updateViewPosition(view);

}

void AppExtension::cursorPosCallback(GLFWwindow *window, double xPos, double yPos) {
    printf("Cursor position: (%f, %f)\n", xPos, yPos);

}

//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include "GameApplication.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

float motion2 = 0;
float zMovement = 0;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float cameraSpeed(.10f);

float yaw = 0, pitch = 0;

double angle = 0;


void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;
    angle += 10.0f * deltaTime;
    if (angle > 360.0f) { angle -= 360.0f; }
    cameraSpeed = 10 * deltaTime;
    //glm::mat4 testMat = glm::mat4(0.0f);


}



void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    GameApplication::keyCallback(window, key, scancode, action, mods);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        std::cout << "space pressed!" << std::endl;
        motion2 += 10;
        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(motion2)), glm::vec3(0.0f, 0.0f, 1.0f));
        vulkanRenderer.updateTriangle(testMat);

    }


    if (key == GLFW_KEY_UP) {
        zMovement += 0.5;
        glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zMovement));
        vulkanRenderer.updateTriangle(translate2);

        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zMovement));
        vulkanRenderer.updateModel(helicopter, translate);

        std::cout << "Up pressed!" << std::endl;
    }



    if (key == GLFW_KEY_DOWN) {
        zMovement -= 0.5;
        glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zMovement));
        vulkanRenderer.updateTriangle(translate2);

        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zMovement));
        vulkanRenderer.updateModel(helicopter, translate);
        std::cout << "Down pressed!" << std::endl;
    }


    if (key == GLFW_KEY_RIGHT) {
        std::cout << "Right pressed!" << std::endl;
        motion += 0.1;

        yaw += 1;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw));
        direction.z = sin(glm::radians(yaw));
        cameraFront = glm::normalize(direction);

    }

    if (key == GLFW_KEY_LEFT) {
        std::cout << "Left pressed!" << std::endl;
        motion -= 0.5f;
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

    vulkanRenderer.updateViewTriangle(view);

}

void AppExtension::cursorPosCallback(GLFWwindow *window, double xPos, double yPos) {
    printf("Cursor position: (%f, %f)\n", xPos, yPos);

    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3((xPos/100), (yPos/100), 0));

    vulkanRenderer.updateModel(helicopter, translate);
}

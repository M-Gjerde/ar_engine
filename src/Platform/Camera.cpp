//
// Created by magnus on 10/12/20.
//

#include "Camera.h"


void Camera::setProjection(glm::mat4 newProjection) {
    projection = newProjection;

}

void Camera::setView(glm::mat4 setView) {
    view = setView;
}

glm::mat4 Camera::getProjection() {
    return projection;
}

glm::mat4 Camera::getView() {
    return view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

}

Camera::Camera() {

    cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
    cameraUp = glm::vec3(0.0f, -1.0f, 0.0f);


    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    projection = glm::perspective(glm::radians(45.0f), (float) viewport.WIDTH / (float) viewport.HEIGHT, 0.1f, 10000.0f);
    projection[1][1] *= -1;

}

void Camera::moveLeft() {
    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

}

void Camera::moveRight() {
    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

}

void Camera::forward() {
    cameraPos += cameraSpeed * cameraFront;

}

void Camera::backward() {
    cameraPos -= cameraSpeed * cameraFront;
}

void Camera::rotateLeft() {
    yaw += 2;
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw));
    direction.z = sin(glm::radians(yaw));
    direction.y = 0.0f;

    cameraFront = glm::normalize(direction);
}

void Camera::rotateRight() {
    yaw -= 2;
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw));
    direction.z = sin(glm::radians(yaw));
    direction.y = 0.0f;
    cameraFront = glm::normalize(direction);
}

void Camera::setRoll(){
    glm::vec3 direction;
    direction.z = sin(glm::radians(roll));
    cameraFront = glm::normalize(direction);
}

void Camera::lookAround(double xPos, double yPos) {

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    double xoffset = xPos - lastX;
    double yoffset = lastY - yPos;
    lastX = xPos;
    lastY = yPos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   -= xoffset;
    pitch -= yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);


}

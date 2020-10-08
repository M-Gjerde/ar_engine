//
// Created by magnus on 10/8/20.
//

#ifndef AR_ENGINE_CAMERA_H
#define AR_ENGINE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Platform//Platform.h"

class Camera {
public:
    float cameraSpeed = 0.25f;
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    Camera();
    void setProjection(glm::mat4 newProjection);
    void setView(glm::mat4 setView);
    glm::mat4 getProjection();
    glm::mat4 getView();

    void moveLeft();
    void moveRight();
    void forward();
    void backward();
    void rotateRight();
    void rotateLeft();

private:
    float yaw = -90;
    glm::mat4 projection{};
    glm::mat4 view{};
};

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

    cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    projection = glm::perspective(glm::radians(45.0f), (float) viewport.WIDTH / (float) viewport.HEIGHT, 0.1f, 100.0f);
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
    yaw -= 1;
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw));
    direction.z = sin(glm::radians(yaw));
    cameraFront = glm::normalize(direction);
}

void Camera::rotateRight() {
    yaw += 1;
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw));
    direction.z = sin(glm::radians(yaw));
    cameraFront = glm::normalize(direction);
}

#endif //AR_ENGINE_CAMERA_H

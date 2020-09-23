//
// Created by magnus on 9/23/20.
//

#ifndef AR_ENGINE_CAMERA_H
#define AR_ENGINE_CAMERA_H

#include <glm/glm.hpp>

class Camera {


public:
    Camera() = default;
    ~Camera() = default;

    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection{};

    bool viewProjectionEnable = false;


    void setProjection(glm::mat4 projection);
    void setView(glm::mat4 view);
    void enableCamera(bool state);

private:



};

void Camera::setProjection(glm::mat4 projection) {
    uboViewProjection.projection = projection;
}

void Camera::setView(glm::mat4 view) {
    uboViewProjection.view = view;

}

void Camera::enableCamera(bool state) {
    viewProjectionEnable = state;
}


#endif //AR_ENGINE_CAMERA_H

//
// Created by magnus on 10/12/20.
//

#ifndef AR_ENGINE_CAMERA_H
#define AR_ENGINE_CAMERA_H



#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "../Platform/Platform.h"


class Camera {
public:
    float cameraSpeed = 0.25f;
    glm::vec3 cameraPos{};
    glm::vec3 cameraFront{};
    glm::vec3 cameraUp{};

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
    void roll();

    float yaw = -90;
    float roll_value = 0;
private:

    glm::mat4 projection{};
    glm::mat4 view{};

};


#endif //AR_ENGINE_CAMERA_H

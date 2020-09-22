//
// Created by magnus on 9/19/20.
//

#ifndef AR_ENGINE_PLAYERCONTROLLER_H
#define AR_ENGINE_PLAYERCONTROLLER_H

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

// Scene Settings
class PlayerController {

public:

    PlayerController() = default;

    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection{};

    float angle = 0;
    float forward = 0;
    bool moving = false;

    void spinOnce(glm::mat4 *object) {
        *object = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(angle)), glm::vec3(0.0f, 1.0f, 0.0f));
    }


};

#endif //AR_ENGINE_PLAYERCONTROLLER_H

//
// Created by magnus on 9/19/20.
//

#ifndef AR_ENGINE_PLAYERCONTROLLER_H
#define AR_ENGINE_PLAYERCONTROLLER_H

#include <glm/glm.hpp>

// Scene Settings
class PlayerController {

public:
    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection{};

};

#endif //AR_ENGINE_PLAYERCONTROLLER_H

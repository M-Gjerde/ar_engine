//
// Created by magnus on 9/23/20.
//

#ifndef AR_ENGINE_OBJECT_H
#define AR_ENGINE_OBJECT_H

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <array>


struct Vertex {
    glm::vec3 pos;
    glm::vec3 col;
    glm::vec2 tex;

};

struct Box {
    glm::vec3 pos;
    glm::vec3 col;
};

const std::vector<Box> boxVertices = {
        {{0.4, -0.4, -0.4}, {1.0f, 0.0f, 0.0f}},
        {{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{-0.4, 0.4, 0.4}, {0.0f, 0.0f, 1.0f}},

        { { -0.4, 0.4, 0.0 }, {0.0f, 0.0f, 1.0f}},
        { { -0.4, -0.4, 0.0 }, {1.0f, 1.0f, 0.0f} },
        { { 0.4, -0.4, 0.0 }, {1.0f, 0.0f, 0.0f} }
};



#endif //AR_ENGINE_OBJECT_H

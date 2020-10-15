//
// Created by magnus on 10/7/20.
//

#ifndef AR_ENGINE_TRIANGLE_H
#define AR_ENGINE_TRIANGLE_H

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

};

struct uboModel {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f,}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

const std::vector<Vertex> meshVertices = {
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint32_t> meshIndices = {
        0, 1, 2,
        2, 3, 0,
};

#endif //AR_ENGINE_TRIANGLE_H

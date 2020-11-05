//
// Created by magnus on 10/7/20.
//

#ifndef AR_ENGINE_TRIANGLE_H
#define AR_ENGINE_TRIANGLE_H

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct uboModel {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

struct FragmentColor {
    glm::vec3 objectColor;
    glm::vec3 lightColor;
    glm::vec3 lightPos;
};


#endif //AR_ENGINE_TRIANGLE_H

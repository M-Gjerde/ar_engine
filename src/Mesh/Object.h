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

struct TriangleVertex {
    glm::vec3 pos;
    glm::vec3 col;
};

struct TriangleModel {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};



#endif //AR_ENGINE_OBJECT_H

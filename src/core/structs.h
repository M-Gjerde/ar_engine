//
// Created by magnus on 9/6/21.
//

#ifndef AR_ENGINE_STRUCTS_H
#define AR_ENGINE_STRUCTS_H


#include <glm/glm.hpp>

// Vertex layout used in this example
struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

class SceneObject {
public:

    SceneObject() = default;
    ~SceneObject() = default;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

};

#endif //AR_ENGINE_STRUCTS_H

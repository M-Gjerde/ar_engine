//
// Created by magnus on 7/5/20.
//

#ifndef UDEMY_VULCAN_UTILITIES_H
#define UDEMY_VULCAN_UTILITIES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>




// Vertex data representation
struct Vertex {
    glm::vec3 pos; // Vertex Position (x, y, z)
    glm::vec3 col; // Vertex color (r, g, b)
    glm::vec2 tex; // Texture coordinates (u, v)
};




#endif //UDEMY_VULCAN_UTILITIES_H

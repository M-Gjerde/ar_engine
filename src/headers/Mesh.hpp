//
// Created by magnus on 7/7/20.
//

#ifndef UDEMY_VULCAN_MESH_HPP
#define UDEMY_VULCAN_MESH_HPP

#define GLFW_INCLUDE_VULKAN

#include "../../external/glfw-3.3.2/include/GLFW/glfw3.h"
#include <vector>
#include "utilities.h"


struct Model {
    glm::mat4 model;
};

class Mesh {
public:
    Mesh();

    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);


    void setModel(glm::mat4 newModel);
    Model getModel();

    // Getters
    [[maybe_unused]] int getVertexCount() const;
    int getIndexCount() const;

    VkBuffer getVertexBuffer();
    VkBuffer getIndexBuffer();

    void destroyBuffers();

    ~Mesh();

private:
    Model model;

    int vertexCount{};
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};

    int indexCount{};
    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    void createVertexBuffer(VkQueue transferQueue,
                            VkCommandPool transferCommandPool, std::vector<Vertex> *vertices);
    void createIndexBuffer(VkQueue transferQueue,
                           VkCommandPool transferCommandPool, std::vector<uint32_t> *indices);


};


#endif //UDEMY_VULCAN_MESH_HPP

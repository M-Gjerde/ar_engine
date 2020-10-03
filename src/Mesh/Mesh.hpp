//
// Created by magnus on 7/7/20.
//

#ifndef UDEMY_VULCAN_MESH_HPP
#define UDEMY_VULCAN_MESH_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "../libs/Utils.h"
#include "Object.h"

struct Model {
    glm::mat4 model;
};

class Mesh {
public:
    Mesh();

    Mesh(Utils::MainDevice newMainDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, int newTexId);

    Mesh(Utils::MainDevice newMainDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool, std::vector<TriangleVertex> *vertices);

    void setModel(glm::mat4 newModel);

    Model getModel();

    // Getters
    [[maybe_unused]] int getVertexCount() const;
    int getIndexCount() const;

    VkBuffer getVertexBuffer();
    VkBuffer getIndexBuffer();
    int getTexId();

    void destroyBuffers();

    ~Mesh();

private:
    Model hModel;

    int texId;

    int vertexCount{};
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};

    int indexCount{};
    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    Utils::MainDevice mainDevice;
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    void createVertexBuffer(VkQueue transferQueue,
                            VkCommandPool transferCommandPool, std::vector<Vertex> *vertices);
    void createIndexBuffer(VkQueue transferQueue,
                           VkCommandPool transferCommandPool, std::vector<uint32_t> *indices);

    void createTriangleVertexBuffer(VkQueue transferQueue,
                                    VkCommandPool transferCommandPool, std::vector<TriangleVertex> *vertices);


};


#endif //UDEMY_VULCAN_MESH_HPP

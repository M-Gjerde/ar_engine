//
// Created by magnus on 10/29/20.
//

#include "MeshModel.h"

#include <cmath>
#include <utility>


#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"


void MeshModel::createMeshFromFile(MainDevice mainDevice, ArModel arModel, const ArModelInfo& arModelInfo) {

    // Get data from object model file
    getDataFromModel(&arModel, arModelInfo);
    std::vector<ArBuffer> modelBuffers(2);
    modelBuffers[0].bufferSize = sizeof(arModel.vertices[0]) * arModel.vertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    modelBuffers[1].bufferSize = sizeof(arModel.indices[0]) * arModel.vertices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create mesh from model I'm just using the constructor. This could be acomplished by a helper class
    mesh = new Mesh(mainDevice, &arModel, modelBuffers);

    arModel1 = arModel;
    indexCount = arModel.indexCount;
}

void MeshModel::createMeshFromModel(MainDevice mainDevice, ArModel arModel) {

    // Get data from object model file
    std::vector<ArBuffer> modelBuffers(2);
    modelBuffers[0].bufferSize = sizeof(arModel.vertices[0]) * arModel.vertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    modelBuffers[1].bufferSize = sizeof(arModel.indices[0]) * arModel.indices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create mesh from model I'm just using the constructor. This could be acomplished by a helper class
    mesh = new Mesh(mainDevice, &arModel, modelBuffers);

    arModel1 = arModel;
    indexCount = arModel.indexCount;

}


void MeshModel::cleanUp(VkDevice device) const {
    vkFreeMemory(device, arModel1.vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, arModel1.vertexBuffer, nullptr);

    vkFreeMemory(device, arModel1.indexBufferMemory, nullptr);
    vkDestroyBuffer(device, arModel1.indexBuffer, nullptr);
}



void MeshModel::setModelFileName(std::string modelName) {
    modelName = std::move(modelName);
}


void MeshModel::getDataFromModel(ArModel *arModel, const ArModelInfo& arModelInfo) {

    // Use library to load in model
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string MODEL_PATH = "../objects/" + arModelInfo.modelName;


    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }


    float nx, ny, nz = 0.0f; // normal for current triangle
    float vx1, vx2, vx3 = 0.f; // vertex 1
    float vy1, vy2, vy3 = 0.f; // vertex 2
    float vz1, vz2, vz3 = 0.f; // vertex 3


    // Load vertex data // TODO Load index data as well
    for (int i = 0; i < shapes.size(); ++i) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); ++j) {
            Vertex vertex{};
            glm::vec3 normal;

            vertex.pos = {attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]};

            if (!attrib.texcoords.empty())
                vertex.texCoord = {attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1]};

            // calculate normals for plane
            if (arModelInfo.generateNormals) {


                vertex.normal = {attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 0],
                        attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 1],
                        attrib.normals[3 * shapes[i].mesh.indices[j].normal_index + 2]};

            }

            arModel->vertices.push_back(vertex);
            arModel->indices.push_back(arModel->indices.size());
        }
    }
    arModel->indexCount = arModel->indices.size();

}



VkBuffer MeshModel::getVertexBuffer() const {
    return arModel1.vertexBuffer;
}

VkBuffer MeshModel::getIndexBuffer() const {
    return arModel1.indexBuffer;
}


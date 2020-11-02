//
// Created by magnus on 10/29/20.
//

#include "MeshModel.h"


#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tinyobj/tiny_obj_loader.h"
#include "../include/structs.h"

Mesh MeshModel::loadModel(MainDevice mainDevice, ArModel *arModel) {

    // Use library to load in model
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string MODEL_PATH = "../objects/" + arModel->modelName;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    // Load vertex data // TODO Load index data as well
    for (int i = 0; i < shapes.size(); ++i) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); ++j) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]};

            if (!attrib.texcoords.empty())
                vertex.texCoord = {attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            arModel->vertices.push_back(vertex);

            arModel->indices.push_back(arModel->indices.size());
        }
    }
    arModel->indexCount = arModel->indices.size();

    // Create two buffers holding vertex and index data
    std::vector<ArBuffer> modelBuffers(2);
    modelBuffers[0].bufferSize = sizeof(arModel->vertices[0]) * arModel->vertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].bufferSize = sizeof(arModel->indices[0]) * arModel->indices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Create mesh from model
    Mesh mesh(mainDevice, arModel, modelBuffers);

    return mesh;
}

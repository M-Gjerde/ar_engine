//
// Created by magnus on 10/29/20.
//

#include "MeshModel.h"

#include <cmath>


#define TINYOBJLOADER_IMPLEMENTATION

#include "../../external/tinyobj/tiny_obj_loader.h"
#include "../include/structs.h"

Mesh MeshModel::loadModel(MainDevice mainDevice, ArModel *arModel, bool generateNormals) {

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
            glm::vec3 normal;

            vertex.pos = {attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]};

            if (!attrib.texcoords.empty())
                vertex.texCoord = {attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1]};


            if (generateNormals) {
                float nx, ny, nz = 0.0f; // normal for current triangle
                float vx1, vx2, vx3 = 0.f; // vertex 1
                float vy1, vy2, vy3 = 0.f; // vertex 2
                float vz1, vz2, vz3 = 0.f; // vertex 3

                float x =attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0];
                float y = attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0];
                float z = attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0];

                switch (j % 3) {
                    case 0:
                        vx1 = x;
                        vy1 = y;
                        vz1 = z;
                        break;
                    case 1:
                        vx2 = x;
                        vy2 = y;
                        vz2 = z;
                        break;
                    case 2:
                        // Defining third point in a triangle
                        vx3 = x;
                        vy3 = y;
                        vz3 = z;
                        float qx, qy, qz, px, py, pz;
                        // Calculate q vector
                        qx = vx2 - vx1;
                        qy = vy2 - vy1;
                        qz = vz2 - vz1;
                        // Calculate p vector
                        px = vx3 - vx1;
                        py = vy3 - vy1;
                        pz = vz3 - vz1;
                        // Calculate normal
                        nx = py * qz - pz * qy;
                        ny = pz * qx - px * qz;
                        nz = px * qy - py * qx;
                        // Scale to unit vector
                        float s = std::sqrt(nx*nx + ny*ny + nz*nz);
                        nx /= s;
                        ny /= s;
                        nz /= s;

                        normal.x = nx;
                        normal.y = ny;
                        normal.z = nz;
                        vertex.normal = normal;

                        break;
                }
            }

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


void MeshModel::calculateNormals() {

}
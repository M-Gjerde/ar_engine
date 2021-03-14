//
// Created by magnus on 10/29/20.
//

#include "MeshModel.h"

#include <cmath>
#include <utility>


#define TINYOBJLOADER_IMPLEMENTATION

#include "../../external/tinyobj/tiny_obj_loader.h"
#include "../include/structs.h"
#include "Descriptors.h"

void MeshModel::loadModel(MainDevice mainDevice, ArModel arModel, ArModelInfo arModelInfo) {

    // Get data from object model file
    getDataFromModel(&arModel, arModelInfo);

    // Create two buffers in GPU memory holding vertex and index data
    std::vector<ArBuffer> modelBuffers(2);
    modelBuffers[0].bufferSize = sizeof(arModel.vertices[0]) * arModel.vertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    modelBuffers[1].bufferSize = sizeof(arModel.indices[0]) * arModel.vertices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create mesh from model
    Mesh mesh(mainDevice, &arModel, modelBuffers);

    arModel1 = arModel;
    indexCount = arModel.indexCount;

}


void MeshModel::cleanUp(VkDevice device) {
    vkFreeMemory(device, arModel1.vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, arModel1.vertexBuffer, nullptr);

    vkFreeMemory(device, arModel1.indexBufferMemory, nullptr);
    vkDestroyBuffer(device, arModel1.indexBuffer, nullptr);
}



void MeshModel::setModelFileName(std::string modelName) {
    modelName = std::move(modelName);
}


void MeshModel::getDataFromModel(ArModel *arModel, ArModelInfo arModelInfo) {

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

                float x = attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0];
                float y = attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1];
                float z = attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2];

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
                        float s = std::sqrt(nx * nx + ny * ny + nz * nz);
                        nx /= s;
                        ny /= s;
                        nz /= s;

                        normal.x = nx;
                        normal.y = ny;
                        normal.z = nz;
                        vertex.normal = -normal;
                        break;
                }
            }
            arModel->vertices.push_back(vertex);
            arModel->indices.push_back(arModel->indices.size());
        }
    }
    arModel->indexCount = arModel->indices.size();

}


const glm::mat4 &MeshModel::getModel() const {
    return model1.model;
}

void MeshModel::setModel(const glm::mat4 &_model) {
    model1.model = _model;
}

const VkBuffer MeshModel::getVertexBuffer() const {
    return arModel1.vertexBuffer;
}

const VkBuffer MeshModel::getIndexBuffer() const {
    return arModel1.indexBuffer;
}

void MeshModel::attachDescriptors(ArDescriptor *arDescriptor, Descriptors* descriptors, Buffer *buffer) {




    // TODO Rewrite usage of vectors like this
    arDescriptor->dataSizes.resize(2);
    arDescriptor->dataSizes[0] = sizeof(uboModel);
    arDescriptor->dataSizes[1] = sizeof(FragmentColor);

    descriptorInfo.descriptorCount = 2;
    std::array<uint32_t, 2> descriptorCounts = {1, 1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 2> bindings = {0, 1};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 2> types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::array<VkShaderStageFlags, 2> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    descriptorInfo.stageFlags = stageFlags.data();


    std::vector<ArBuffer> buffers(2);
    // Create and fill buffers
    for (int j = 0; j < 2; ++j) {
        buffers[j].bufferSize = sizes[j];
        buffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffers[j].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffers[j].bufferProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        buffer->createBuffer(&buffers[j]);
        arDescriptor->buffer.push_back(buffers[j].buffer);
        arDescriptor->bufferMemory.push_back(buffers[j].bufferMemory);
    }

    descriptors->createDescriptors(descriptorInfo, arDescriptor);

}


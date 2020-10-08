//
// Created by magnus on 10/8/20.
//

#ifndef AR_ENGINE_MESH_H
#define AR_ENGINE_MESH_H


#include "../include/BufferCreation.h"

class Mesh : Buffer {

public:

    explicit Mesh(ArEngine engine, StandardModel *standardModel, std::vector<ArBuffer> newArBuffer);
    void cleanUp();
    ~Mesh();
private:
    VkDevice device;
    ArBuffer vertexBuffer{};
    ArBuffer indexBuffer{};

    StandardModel modelInfo{};

    void createVertexBuffer();

    void createIndexBuffer();
};


#endif //AR_ENGINE_MESH_H

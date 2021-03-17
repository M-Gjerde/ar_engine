//
// Created by magnus on 10/8/20.
//

#ifndef AR_ENGINE_MESH_H
#define AR_ENGINE_MESH_H


#include "Buffer.h"
#include <cstring>

class Mesh : Buffer {

public:

    explicit Mesh(MainDevice mainDevice, ArModel *standardModel, std::vector<ArBuffer> newArBuffer);

    void cleanUp();
    ~Mesh();
private:
    VkDevice device;
    ArBuffer indexBuffer{};
    ArBuffer vertexBuffer{};
    ArModel modelInfo{};

    void createVertexBuffer();
    void createIndexBuffer();

};


#endif //AR_ENGINE_MESH_H

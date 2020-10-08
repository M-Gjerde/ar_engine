//
// Created by magnus on 10/8/20.
//

#ifndef AR_ENGINE_MESH_H
#define AR_ENGINE_MESH_H


#include "../include/BufferCreation.h"

class Mesh : Buffer {

public:


private:

    void createVertexBuffer(ArBuffer *arBuffer);

    void createIndexBuffer();
};


#endif //AR_ENGINE_MESH_H

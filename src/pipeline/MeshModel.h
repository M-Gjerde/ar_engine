//
// Created by magnus on 10/29/20.
//

#ifndef AR_ENGINE_MESHMODEL_H
#define AR_ENGINE_MESHMODEL_H


#include <string>
#include "../include/structs.h"
#include "Mesh.h"
#include "../../external/tinyobj/tiny_obj_loader.h"
#include "Descriptors.h"

class MeshModel {

public:

    void loadModel(MainDevice mainDevice, ArModel arModel, ArModelInfo arModelInfo);

    void setModelFileName(std::string modelName);
    void setModel(const glm::mat4 &_model);
    const glm::mat4 &getModel() const;

    VkBuffer const getIndexBuffer() const;
    VkBuffer const getVertexBuffer() const;

    void cleanUp(VkDevice device);

    uint32_t indexCount = -1;

    void attachDescriptors(ArDescriptor *arDescriptor, Descriptors *descriptors, Buffer *buffer);

private:
    uboModel model1;
    ArModel arModel1;

    void getDataFromModel(ArModel *arModel, ArModelInfo arModelInfo);

};


#endif //AR_ENGINE_MESHMODEL_H

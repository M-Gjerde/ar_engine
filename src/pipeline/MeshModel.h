//
// Created by magnus on 10/29/20.
//

#ifndef AR_ENGINE_MESHMODEL_H
#define AR_ENGINE_MESHMODEL_H


#include <string>
#include "../include/structs.h"
#include "Mesh.h"

class MeshModel {

public:

    Mesh loadModel(MainDevice mainDevice, ArModel *arModel, bool generateNormals);

    void setModel(std::string modelName);

private:

    std::string modelName;

    void calculateNormals();
};


#endif //AR_ENGINE_MESHMODEL_H

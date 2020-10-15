//
// Created by magnus on 10/7/20.
//

#ifndef AR_ENGINE_DESCRIPTORS_H
#define AR_ENGINE_DESCRIPTORS_H


#include "../include/structs.h"
#include <stdexcept>

class Descriptors {
public:

    explicit Descriptors(const ArEngine& engine);

    void createDescriptors(ArDescriptor *pDescriptor);
    void createDescriptorsSampler(ArDescriptor *pDescriptor, ArTextureSampler pTextureSampler);

    void cleanUp(ArDescriptor arDescriptor);

private:
    VkDevice device;
    ArDescriptor mArDescriptor;
    ArTextureSampler arTextureSampler;

    void createSetLayout();
    void createSetPool();
    void createDescriptorSets();

    void createTextureSamplerDescriptor();


};


#endif //AR_ENGINE_DESCRIPTORS_H

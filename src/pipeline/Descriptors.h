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
    void createDescriptorsSampler(ArDescriptor *pDescriptor, ArTextureImage pTextureSampler);

    void cleanUp(ArDescriptor arDescriptor);

    void lightDescriptors(ArDescriptor *pDescriptor);

private:
    VkDevice device;
    ArDescriptor mArDescriptor;
    ArTextureImage arTextureSampler{};

    void createSetLayout();
    void createSetPool();
    void createDescriptorSets();

    void updateTextureSamplerDescriptor();


    void fragmentSetLayout();
    void fragmentDescriptorSet();

    void createLightPool();
};


#endif //AR_ENGINE_DESCRIPTORS_H

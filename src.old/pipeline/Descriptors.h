//
// Created by magnus on 10/7/20.
//

#ifndef AR_ENGINE_DESCRIPTORS_H
#define AR_ENGINE_DESCRIPTORS_H


#include "../include/structs.h"
#include <stdexcept>

class Descriptors {
public:

    explicit Descriptors(const ArEngine &engine);

    void createDescriptorsSampler(ArDescriptor *pDescriptor, ArTextureImage pTextureSampler);
    void cleanUp(ArDescriptor arDescriptor);
    void createDescriptors(ArDescriptorInfo descriptorInfo, ArDescriptor *pDescriptor);

private:
    VkDevice device;
    ArDescriptor mArDescriptor;
    ArTextureImage arTextureSampler{};


    void updateTextureSamplerDescriptor();
    void fragmentSetLayout();
    void fragmentDescriptorSet();
    void createLightPool();


    void createDescriptorsSetLayout(ArDescriptorInfo info, ArDescriptor *pDescriptor);
    void createDescriptorSets(ArDescriptorInfo info, ArDescriptor *pDescriptor);
    void createSetPool(ArDescriptorInfo info, ArDescriptor *pDescriptor);

};


#endif //AR_ENGINE_DESCRIPTORS_H

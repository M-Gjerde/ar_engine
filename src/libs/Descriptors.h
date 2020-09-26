//
// Created by magnus on 9/26/20.
//

#ifndef AR_ENGINE_DESCRIPTORS_H
#define AR_ENGINE_DESCRIPTORS_H

#include "Utils.h"

class Descriptors {
public:

    Descriptors(Utils::MainDevice newMainDevice);
    ~Descriptors() = default;

    void createDescriptorSetLayout(VkDescriptorSetLayout * descriptorSetLayout);
    void createDescriptorSetPool(VkDescriptorPool *descriptorPool, uint32_t maxSets);
    void createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool,
                             VkBuffer bufferBinding, VkDescriptorSet *descriptorSet, size_t bufferInfoRange);



private:

    Utils::MainDevice mainDevice{};

};


#endif //AR_ENGINE_DESCRIPTORS_H

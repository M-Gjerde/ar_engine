//
// Created by magnus on 9/26/20.
//

#ifndef AR_ENGINE_DESCRIPTORS_H
#define AR_ENGINE_DESCRIPTORS_H

#include "Utils.h"

class Descriptors {
public:

    explicit Descriptors(Utils::MainDevice newMainDevice);
    ~Descriptors() = default;

    void createDescriptorSetLayout(VkDescriptorSetLayout * descriptorSetLayout) const;
    void createDescriptorSetPool(VkDescriptorPool *descriptorPool, uint32_t maxSets) const;
    void createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool,
                             VkBuffer bufferBinding, VkDescriptorSet *descriptorSet, size_t bufferInfoRange) const;


    void clean(Utils::UniformBuffer uniformBuffer);

    void createDescriptorPool(VkDescriptorType type, uint32_t count, VkDescriptorPool *descriptorPool,
                              VkAllocationCallbacks *allocator) const;

    void createDescriptorPool(std::vector<VkDescriptorType> types, const uint32_t *descriptorCount, uint32_t maxSets,
                              VkDescriptorPool *descriptorPool, VkAllocationCallbacks *allocator) const;


    void createUniformBufferDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout, VkShaderStageFlags flags,
                                                uint32_t binding, VkAllocationCallbacks *allocator) const;

    void createInputAttachmentsDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout, VkShaderStageFlags flags,
                                                   uint32_t *bindings, VkAllocationCallbacks *allocator);


    void createSamplerDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout, VkShaderStageFlags flags,
                                          uint32_t binding, VkAllocationCallbacks *allocator) const;

    void createInputDescriptorSets();
private:

    Utils::MainDevice mainDevice{};


};


#endif //AR_ENGINE_DESCRIPTORS_H

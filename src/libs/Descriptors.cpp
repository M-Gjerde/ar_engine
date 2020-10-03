//
// Created by magnus on 9/26/20.
//

#include <stdexcept>
#include "Descriptors.h"

Descriptors::Descriptors(Utils::MainDevice newMainDevice) {
    mainDevice = newMainDevice;
}

void Descriptors::createDescriptorSetLayout(VkDescriptorSetLayout *descriptorSetLayout) const {

    // Model binding info
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpLayoutBinding.descriptorCount = 1;
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vpLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {vpLayoutBinding};
    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                               // Array of binding infos


    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, descriptorSetLayout);
    if (result != VK_SUCCESS) throw std::runtime_error("Could not create Descriptor set layout");
}

void Descriptors::createDescriptorSetPool(VkDescriptorPool *descriptorPool, uint32_t descriptorCount) const {
    // CREATE UNIFORM DESCRIPTOR POOL FOR TRIANGLE
    VkDescriptorPoolSize  poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
    vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vkDescriptorPoolCreateInfo.maxSets = descriptorCount;
    vkDescriptorPoolCreateInfo.poolSizeCount = 1;             // Amount of pool sizes being passed
    vkDescriptorPoolCreateInfo.pPoolSizes = &poolSize;

    // Create descriptor pool

    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &vkDescriptorPoolCreateInfo, nullptr, descriptorPool);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool");

}

void Descriptors::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool,
                                      VkBuffer bufferBinding, VkDescriptorSet *descriptorSet, size_t bufferInfoRange) const {


    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &descriptorSetAllocateInfo, descriptorSet);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to allocate descriptor sets");

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = bufferBinding;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferInfoRange;


    // Update all descriptor set buffer bindings

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = *descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &writeDescriptorSet, 0, nullptr);

}

void Descriptors::clean(Utils::UniformBuffer uniformBuffer) {


    vkDestroyDescriptorPool(mainDevice.logicalDevice, uniformBuffer.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, uniformBuffer.descriptorSetLayout, nullptr);

}

void Descriptors::createDescriptorPool(VkDescriptorType type, uint32_t count, VkDescriptorPool *descriptorPool, VkAllocationCallbacks* allocator) const {

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = type;
    poolSize.descriptorCount = count;

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = count;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;

    // Create descriptor pool
    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, allocator, descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");


}

void Descriptors::createDescriptorPool(std::vector<VkDescriptorType> types, const uint32_t *descriptorCount, uint32_t maxSets,
                                       VkDescriptorPool *descriptorPool, VkAllocationCallbacks *allocator) const {

    std::vector<VkDescriptorPoolSize> descriptorPoolSize(types.size());

    for (int i = 0; i < types.size(); ++i) {
        descriptorPoolSize[i].type = types[i];
        descriptorPoolSize[i].descriptorCount = descriptorCount[i];
    }

    VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
    inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    inputPoolCreateInfo.maxSets = maxSets;
    inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());
    inputPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &inputPoolCreateInfo, allocator, descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");


}

void Descriptors::createUniformBufferDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout,
                                                         VkShaderStageFlags flags, uint32_t binding,
                                                         VkAllocationCallbacks *allocator) const {

    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = flags;
    layoutBinding.pImmutableSamplers = nullptr;

    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;                                   // Number of binding infos
    layoutCreateInfo.pBindings = &layoutBinding;                         // Array of binding infos


    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, allocator,
                                                  pDescriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

}

void Descriptors::createInputAttachmentsDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout,
                                                            VkShaderStageFlags flags, uint32_t* bindings,
                                                            VkAllocationCallbacks *allocator) {

    // CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
    // Color input binding
    VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
    colorInputLayoutBinding.binding = 0;
    colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding.descriptorCount = 1;
    colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth input binding
    VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
    depthInputLayoutBinding.binding = 1;
    depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputLayoutBinding.descriptorCount = 1;
    depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> inputBindings = {colorInputLayoutBinding, depthInputLayoutBinding};


    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
    layoutCreateInfo.pBindings = inputBindings.data();


    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, allocator,
                                                  pDescriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

}

void Descriptors::createSamplerDescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout,
                                                         VkShaderStageFlags flags, uint32_t binding,
                                                         VkAllocationCallbacks *allocator) const {

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = binding;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = flags;

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor set layout
    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &textureLayoutCreateInfo, allocator,
                                         pDescriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor Set Layout");



}

void Descriptors::createInputDescriptorSets() {

}






//
// Created by magnus on 9/26/20.
//

#include <stdexcept>
#include "Descriptors.h"

Descriptors::Descriptors(Utils::MainDevice newMainDevice) {
    mainDevice = newMainDevice;
}

void Descriptors::createDescriptorSetLayout(VkDescriptorSetLayout *descriptorSetLayout) {

    // Model binding info
    VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 0;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {modelLayoutBinding};
    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                               // Array of binding infos


    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, descriptorSetLayout);
    if (result != VK_SUCCESS) throw std::runtime_error("Could not create Descriptor set layout");
}

void Descriptors::createDescriptorSetPool(VkDescriptorPool *descriptorPool, uint32_t maxSets) {


    // CREATE UNIFORM DESCRIPTOR POOL FOR TRIANGLE
    VkDescriptorPoolSize  poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = maxSets;

    VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
    vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vkDescriptorPoolCreateInfo.maxSets = maxSets;
    vkDescriptorPoolCreateInfo.poolSizeCount = 1;             // Amount of pool sizes being passed
    vkDescriptorPoolCreateInfo.pPoolSizes = &poolSize;

    // Create descriptor pool

    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &vkDescriptorPoolCreateInfo, nullptr, descriptorPool);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool");

}

void Descriptors::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool,
                                      VkBuffer bufferBinding, VkDescriptorSet *descriptorSet, size_t bufferInfoRange) {


    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &descriptorSetAllocateInfo, descriptorSet);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to allocate descriptor sets");

    VkDescriptorBufferInfo triangleBufferInfo = {};
    triangleBufferInfo.buffer = bufferBinding;
    triangleBufferInfo.offset = 0;
    triangleBufferInfo.range = bufferInfoRange;


    // Update all descriptor set buffer bindings

    VkWriteDescriptorSet triangleSetWrite = {};
    triangleSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    triangleSetWrite.dstSet = *descriptorSet;
    triangleSetWrite.dstBinding = 0;
    triangleSetWrite.dstArrayElement = 0;
    triangleSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    triangleSetWrite.descriptorCount = 1;
    triangleSetWrite.pBufferInfo = &triangleBufferInfo;

    vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &triangleSetWrite, 0, nullptr);

}



//
// Created by magnus on 10/7/20.
//


#include "Descriptors.h"
#include "../include/triangle.h"

Descriptors::Descriptors(const ArEngine &engine) {
    device = engine.mainDevice.device;
}

void Descriptors::createDescriptors(ArDescriptor *pDescriptor) {
    mArDescriptor = *pDescriptor;
    createSetLayout();
    createSetPool();
    createDescriptorSets();

    *pDescriptor = mArDescriptor;
}

void Descriptors::createSetLayout() {

    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // VP binding info Create descriptor layout bindings
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
    uboLayoutBinding.descriptorCount = 1;                                   // Number of descriptors for binding
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;               // Shader stage to bind to
    uboLayoutBinding.pImmutableSamplers = nullptr;                          // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed


    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {uboLayoutBinding};
    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                               // Array of binding infos

    // Create descriptor set layout
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr,
                                                  &mArDescriptor.descriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");
}

void Descriptors::createSetPool() {

    // CREATE UNIFORM DESCRIPTOR POOL
    // Type of descriptors + how many DESCRIPTORS, not Descriptor sets (combined makes the pool size)
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(mArDescriptor.descriptorSets.size());

    /*
    VkDescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDynamicUniformBuffer.size());
*/
    std::vector<VkDescriptorPoolSize> poolList = {vpPoolSize,};

    // Data to create descriptor pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(mArDescriptor.descriptorSets.size());       // Maximum nmber of descriptor sets that can be created from pool
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolList.size());        // Amount of pool sizes being passed
    poolCreateInfo.pPoolSizes = poolList.data();                                  // Pool sizes to create pool with

    // Create descriptor pool
    VkResult result = vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &mArDescriptor.descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");

}

void Descriptors::createDescriptorSets() {


    std::vector<VkDescriptorSetLayout> setLayouts(mArDescriptor.descriptorSets.size(),
                                                  mArDescriptor.descriptorSetLayout);

    //Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = mArDescriptor.descriptorPool;                                     // Pool to allocate descriptor set from
    setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(mArDescriptor.descriptorSets.size());  // Number of sets to allocate
    setAllocateInfo.pSetLayouts = setLayouts.data();                                     // Layouts to use to allocates sets(1:1 relationship)

    // Allocate descriptor sets (multiple)

    VkResult result = vkAllocateDescriptorSets(device, &setAllocateInfo, mArDescriptor.descriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets");

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < mArDescriptor.descriptorSets.size(); i++) {
        // Buffer info and data offset info
        // VIEW PROJECTION DESCRIPTOR
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = mArDescriptor.buffer[i];                // Buffer to get data from
        vpBufferInfo.offset = 0;                               // Offset into the data
        vpBufferInfo.range = sizeof(uboModel);                      // Size of the data that is going to be bound to the descriptor set


        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = mArDescriptor.descriptorSets[i];                         // Descriptor set to update
        vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader
        vpSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor we are updating
        vpSetWrite.descriptorCount = 1;                                // Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind


        std::vector<VkWriteDescriptorSet> writeDescriptorSetlist = {vpSetWrite};
        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSetlist.size()),
                               writeDescriptorSetlist.data(), 0, nullptr);
    }
}

void Descriptors::cleanUp(ArDescriptor arDescriptor) {

    vkDestroyDescriptorSetLayout(device, arDescriptor.descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, arDescriptor.descriptorPool, nullptr);

    for (int i = 0; i < arDescriptor.buffer.size(); ++i) {
        vkFreeMemory(device, arDescriptor.bufferMemory[i], nullptr);
        vkDestroyBuffer(device, arDescriptor.buffer[i],nullptr);

    }

}

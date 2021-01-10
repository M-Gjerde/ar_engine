//
// Created by magnus on 10/7/20.
//


#include <array>
#include "Descriptors.h"

Descriptors::Descriptors(const ArEngine &engine) {
    device = engine.mainDevice.device;
}

void Descriptors::createDescriptors(std::vector<ArDescriptorInfo> descriptorInfo, ArDescriptor *pDescriptor) {
    createSetLayout(descriptorInfo, pDescriptor);
    createSetPool(descriptorInfo, pDescriptor);
    createDescriptorSets(pDescriptor);
}

void Descriptors::lightDescriptors(ArDescriptor *pDescriptor) {
    mArDescriptor = *pDescriptor;

    fragmentSetLayout();
    createLightPool();
    fragmentDescriptorSet();

    // Return descriptors
    *pDescriptor = mArDescriptor;
}


void Descriptors::createDescriptorsSampler(ArDescriptor *pDescriptor, ArTextureImage pTextureSampler) {
    arTextureSampler = pTextureSampler;
    mArDescriptor = *pDescriptor;
    updateTextureSamplerDescriptor();

    // Return descriptors
    *pDescriptor = mArDescriptor;
}

void Descriptors::createLightPool() {

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
    std::vector<VkDescriptorPoolSize> poolList = {vpPoolSize};

    // Data to create descriptor pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(mArDescriptor.descriptorSets.size() +
                                                   1);       // Maximum number of descriptor sets that can be created from pool
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolList.size());        // Amount of pool sizes being passed
    poolCreateInfo.pPoolSizes = poolList.data();                                  // Pool sizes to create pool with

    // Create descriptor pool
    VkResult result = vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &mArDescriptor.descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");

}

// TODO Possible remake of this. Make this dynamic if possible
void Descriptors::fragmentSetLayout() {
    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // VP binding info Create descriptor layout bindings
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
    uboLayoutBinding.descriptorCount = 1;                                   // Number of descriptors for binding
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;              // Shader stage to bind to
    uboLayoutBinding.pImmutableSamplers = nullptr;                          // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed

    // Color layout binding
    VkDescriptorSetLayoutBinding colorLayoutBinding{};
    colorLayoutBinding.binding = 1;
    colorLayoutBinding.descriptorCount = 1;
    colorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorLayoutBinding.pImmutableSamplers = nullptr;
    colorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {uboLayoutBinding, colorLayoutBinding};
    // Create descriptor set layout with given bindings
    std::vector<VkDescriptorSetLayoutCreateInfo> layoutCreateInfos;
    layoutCreateInfos.resize(3); //TODO This may break the program if more descriptorsSetLayouts wants to be created
    layoutCreateInfos[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfos[0].bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfos[0].pBindings = layoutBindings.data();                               // Array of binding infos

    // Create descriptor set layout with given bindings
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings2 = {colorLayoutBinding};
    layoutCreateInfos[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfos[1].bindingCount = static_cast<uint32_t>(layoutBindings2.size());     // Number of binding infos
    layoutCreateInfos[1].pBindings = layoutBindings2.data();                               // Array of binding infos

    for (int i = 0; i < mArDescriptor.descriptorSetLayouts.size(); ++i) {
        VkResult result = vkCreateDescriptorSetLayout(device, &layoutCreateInfos[i], nullptr,
                                                      &mArDescriptor.descriptorSetLayouts[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create Descriptor set layout");
    }


}


void Descriptors::fragmentDescriptorSet() {

    //std::vector<VkDescriptorSetLayout> setLayouts = {mArDescriptor.descriptorSetLayout, mArDescriptor.descriptorSetLayout2};
    std::vector<VkDescriptorSetLayout> setLayouts = mArDescriptor.descriptorSetLayouts; //TODO fix
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



    // Buffer info and data offset info
    // VIEW PROJECTION DESCRIPTOR
    VkDescriptorBufferInfo vpBufferInfo = {};
    vpBufferInfo.buffer = mArDescriptor.buffer[0];                // Buffer to get data from
    vpBufferInfo.offset = 0;                                      // Offset into the data
    vpBufferInfo.range = sizeof(uboModel);                        // Size of the data that is going to be bound to the descriptor set


    // Data about connection between binding and buffer
    VkWriteDescriptorSet vpSetWrite = {};
    vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vpSetWrite.dstSet = mArDescriptor.descriptorSets[0];                         // Descriptor set to update
    vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader
    vpSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
    vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor we are updating
    vpSetWrite.descriptorCount = 1;                                // Amount to update
    vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind

    // FRAGMENT COLOR DESCRIPTOR
    VkDescriptorBufferInfo colorBufferInfo = {};
    colorBufferInfo.buffer = mArDescriptor.buffer[1];              // Buffer to get data from
    colorBufferInfo.offset = 0;                                    // Offset into the data
    colorBufferInfo.range = sizeof(FragmentColor);                 // Size of the data that is going to be bound to the descriptor set


    // Data about connection between binding and buffer
    VkWriteDescriptorSet colorSetWrite = {};
    colorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    colorSetWrite.dstSet = mArDescriptor.descriptorSets[1];                         // Descriptor set to update
    colorSetWrite.dstBinding = 1;                                     // Binding to update (matches with binding on layout/shader
    colorSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
    colorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor we are updating
    colorSetWrite.descriptorCount = 1;                                // Amount to update
    colorSetWrite.pBufferInfo = &colorBufferInfo;                        // Information about buffer data to bind


    std::vector<VkWriteDescriptorSet> writeDescriptorSetlist;
    writeDescriptorSetlist.push_back(vpSetWrite);
    if(mArDescriptor.dataSizes.size() > 1)
        writeDescriptorSetlist.push_back(colorSetWrite);
    // Update the descriptor sets with new buffer/binding info
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSetlist.size()), writeDescriptorSetlist.data(),
                           0, nullptr);


}

void Descriptors::createSetLayout(std::vector<ArDescriptorInfo> info, ArDescriptor *pDescriptor) {

    std::vector<VkDescriptorSetLayoutBinding> uboLayoutBinding(info.size());

    for (int i = 0; i < 2; ++i) {
        // UNIFORM VALUES DESCRIPTOR SET LAYOUT
        uboLayoutBinding[i].binding = info[i].binding;                                // Binding point in shader (designated by binding number in shader)
        uboLayoutBinding[i].descriptorType = info[i].descriptorType;                  // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
        uboLayoutBinding[i].descriptorCount = info[i].descriptorCount;                // Number of descriptors for binding
        uboLayoutBinding[i].stageFlags = info[i].stageFlags;                          // Shader stage to bind to
        uboLayoutBinding[i].pImmutableSamplers = nullptr;                             // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 2;                                               // Number of binding infos
    layoutCreateInfo.pBindings = uboLayoutBinding.data();                               // Array of binding infos

    // Create descriptor set layout
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr,
                                                  &pDescriptor->descriptorSetLayouts[0]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

    // Create a 2nd Descriptor set layout for output image using template from first descriptor set
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;                                               // Number of binding infos
    layoutCreateInfo.pBindings = &uboLayoutBinding[0];                               // Array of binding infos
    // Create descriptor set layout
    result = vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr,
                                                  &pDescriptor->descriptorSetLayouts[1]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

}

void Descriptors::createSetPool(std::vector<ArDescriptorInfo> info, ArDescriptor *pDescriptor) {

    // CREATE UNIFORM DESCRIPTOR POOL
    // Type of descriptors + how many DESCRIPTORS, not Descriptor sets (combined makes the pool size)
    VkDescriptorPoolSize descriptorPoolSize = {};
    descriptorPoolSize.type = info[0].descriptorType;
    descriptorPoolSize.descriptorCount = 3;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(info.size()); // we only need to allocate one descriptor set from the pool.
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

    // create descriptor pool.
    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, NULL, &pDescriptor->descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor pool");

}

void Descriptors::createDescriptorSets(ArDescriptor *pDescriptor) {

    //Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = pDescriptor->descriptorPool;                                         // Pool to allocate descriptor set from
    setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(pDescriptor->descriptorSetLayouts.size()); // Number of sets to allocate
    setAllocateInfo.pSetLayouts = pDescriptor->descriptorSetLayouts.data();                               // Layouts to use to allocates sets(1:1 relationship)

    // Allocate descriptor sets (multiple)

    VkResult result = vkAllocateDescriptorSets(device, &setAllocateInfo, pDescriptor->descriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets");


    // Update all of descriptor set buffer bindings for FIRST DESCRIPTOR SET
    for (size_t i = 0; i < 2; i++) {
        // Buffer info and data offset info
        // VIEW PROJECTION DESCRIPTOR
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = pDescriptor->buffer[i];                               // Buffer to get data from
        vpBufferInfo.offset = 0;                                                    // Offset into the data
        vpBufferInfo.range = pDescriptor->dataSizes[0];                 // Size of the data that is going to be bound to the descriptor set

        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = pDescriptor->descriptorSets[0];            // Descriptor set to update
        vpSetWrite.dstBinding = i;                                     // Binding to update (matches with binding on layout/shader
        vpSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // Type of descriptor we are updating
        vpSetWrite.descriptorCount = 1;                                // Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind

        std::vector<VkWriteDescriptorSet> writeDescriptorSetlist = {vpSetWrite};
        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSetlist.size()),
                               writeDescriptorSetlist.data(), 0, nullptr);

    }

    // UPDATE SECOND DESCRIPTOR SET
    // Buffer info and data offset info
    // VIEW PROJECTION DESCRIPTOR
    VkDescriptorBufferInfo vpBufferInfo = {};
    vpBufferInfo.buffer = pDescriptor->buffer[2];                               // Buffer to get data from
    vpBufferInfo.offset = 0;                                                    // Offset into the data
    vpBufferInfo.range = pDescriptor->dataSizes[0];                      // Size of the data that is going to be bound to the descriptor set

    // Data about connection between binding and buffer
    VkWriteDescriptorSet vpSetWrite = {};
    vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vpSetWrite.dstSet = pDescriptor->descriptorSets[1];            // Descriptor set to update
    vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader
    vpSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
    vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // Type of descriptor we are updating
    vpSetWrite.descriptorCount = 1;                                // Amount to update
    vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind

    std::vector<VkWriteDescriptorSet> writeDescriptorSetlist = {vpSetWrite};
    // Update the descriptor sets with new buffer/binding info
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSetlist.size()),
                           writeDescriptorSetlist.data(), 0, nullptr);
}

void Descriptors::cleanUp(ArDescriptor arDescriptor) {

    for (int i = 0; i < arDescriptor.buffer.size(); ++i) {
        vkFreeMemory(device, arDescriptor.bufferMemory[i], nullptr);
        vkDestroyBuffer(device, arDescriptor.buffer[i], nullptr);

    }

    for (int i = 0; i < arDescriptor.descriptorSetLayouts.size(); ++i) {
        vkDestroyDescriptorSetLayout(device, arDescriptor.descriptorSetLayouts[i], nullptr);
    }

    vkDestroyDescriptorPool(device, arDescriptor.descriptorPool, nullptr);

}

void Descriptors::updateTextureSamplerDescriptor() {


    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < mArDescriptor.descriptorSets.size(); i++) {
        // Buffer info and data offset info
        // VIEW PROJECTION DESCRIPTOR
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = arTextureSampler.textureImageView;
        imageInfo.sampler = arTextureSampler.textureSampler;

        // Data about connection between binding and buffer
        VkWriteDescriptorSet samplerSetWrite = {};
        samplerSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerSetWrite.dstSet = mArDescriptor.descriptorSets[i];                   // Descriptor set to update
        samplerSetWrite.dstBinding = 1;                                             // Binding to update (matches with binding on layout/shader
        samplerSetWrite.dstArrayElement = 0;                                        // Index in the array we want to update
        samplerSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Type of descriptor we are updating
        samplerSetWrite.descriptorCount = 1;                                        // Amount to update
        samplerSetWrite.pImageInfo = &imageInfo;                                    // Information about buffer data to bind


        std::vector<VkWriteDescriptorSet> writeDescriptorSetlist = {samplerSetWrite};
        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSetlist.size()),
                               writeDescriptorSetlist.data(), 0, nullptr);
    }
}

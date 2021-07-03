//
// Created by magnus on 10/7/20.
//


#include <array>
#include "Descriptors.h"

Descriptors::Descriptors(const ArEngine &engine) {
    device = engine.mainDevice.device;
}

void Descriptors::createDescriptors(ArDescriptorInfo descriptorInfo, ArDescriptor *pDescriptor) {
    //createSetLayouts(descriptorInfo, pDescriptor);
    createDescriptorsSetLayout(descriptorInfo, pDescriptor);
    createSetPool(descriptorInfo, pDescriptor);
    createDescriptorSets(descriptorInfo, pDescriptor);
}


void Descriptors::createDescriptorsSampler(ArDescriptor *pDescriptor, ArTextureImage pTextureSampler) {
    arTextureSampler = pTextureSampler;
    mArDescriptor = *pDescriptor;
    updateTextureSamplerDescriptor();

    // Return descriptors
    *pDescriptor = mArDescriptor;
}


void Descriptors::createDescriptorsSetLayout(ArDescriptorInfo info, ArDescriptor *pDescriptor) {

    // allocate memory for Layouts
    for (int i = 0; i < info.descriptorSetLayoutCount; ++i) {
        pDescriptor->pDescriptorSetLayouts = new VkDescriptorSetLayout();
    }
    // copy pointer 1st position
    VkDescriptorSetLayout *pOrig = &pDescriptor->pDescriptorSetLayouts[0];

    // Create multiple descriptors in one set
    // TODO Create assertions
    for (int i = 0; i < info.descriptorSetCount; ++i) {
        std::vector<VkDescriptorSetLayoutBinding> layoutBinding(*info.pDescriptorSplitCount);

        for (int index = 0; index < *info.pDescriptorSplitCount; ++index) {
            // UNIFORM VALUES DESCRIPTOR SET LAYOUT
            layoutBinding[index].binding = *info.pBindings;                                // Binding point in shader (designated by binding number in shader)
            layoutBinding[index].descriptorType = *info.pDescriptorType;                   // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
            layoutBinding[index].descriptorCount = 1;                                      // Number of descriptors for binding // Default 1 descriptor per binding
            layoutBinding[index].stageFlags = *info.stageFlags;                            // Shader stage to bind to
            layoutBinding[index].pImmutableSamplers = nullptr;                             // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed

            // Increment pointers
            info.pBindings++;
            info.pDescriptorType++;
            info.stageFlags++;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = *info.pDescriptorSplitCount;                        // Number of binding infos
        layoutInfo.pBindings = layoutBinding.data();                                  // Array of binding infos
        layoutInfo.pNext = nullptr;
        layoutInfo.flags = 0;
        // alloc memory

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                        pDescriptor->pDescriptorSetLayouts) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a descriptor set layout");
        // for more layouts increment layout pointer
        if (i != info.descriptorSetLayoutCount - 1) {
            pDescriptor->pDescriptorSetLayouts++;
            // increment to next descriptor set
            info.pDescriptorSplitCount++;
        }
    }
    // reset pointer to first layout
    pDescriptor->pDescriptorSetLayouts = pOrig;
}


void Descriptors::createSetPool(ArDescriptorInfo info, ArDescriptor *pDescriptor) {
    // TODO CREATE VARIATIONS FOR POOL TYPE
    // CREATE UNIFORM DESCRIPTOR POOL
    // Type of descriptors + how many DESCRIPTORS, not Descriptor sets (combined makes the pool size)
    std::vector<VkDescriptorPoolSize> descriptorPoolSize(info.descriptorPoolCount);
    for (int i = 0; i < info.descriptorPoolCount; ++i) {
        descriptorPoolSize[i].type = *info.pDescriptorType;
        descriptorPoolSize[i].descriptorCount = reinterpret_cast<uint32_t>(info.descriptorCount); // TODO CREATE ONLY NECESSARY POOL SIZE

        // Increment pointer to next type
        for (int j = 0; j < *info.pDescriptorSplitCount; ++j) {
            info.pDescriptorType++;
        }
        info.pDescriptorSplitCount++;
    }



    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = info.descriptorCount;                                        // We are not going to allocate more
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());      // Number of pools
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

    // create descriptor pool.
    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, NULL, &pDescriptor->pDescriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor pool");

}

void Descriptors::createDescriptorSets(ArDescriptorInfo info, ArDescriptor *pDescriptor) {

    //Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = pDescriptor->pDescriptorPool;                                 // Pool to allocate descriptor set from
    setAllocateInfo.descriptorSetCount = info.descriptorSetCount;                                   // Number of sets to allocate
    setAllocateInfo.pSetLayouts = pDescriptor->pDescriptorSetLayouts;                               // Layouts to use to allocates sets(1:1 relationship)

    // TODO possible remake of this. Resizing and setting pDescriptor parameters here seems odd
    pDescriptor->descriptorSets.resize(info.descriptorSetCount);
    pDescriptor->descriptorSetLayoutCount = info.descriptorSetLayoutCount;
    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(device, &setAllocateInfo, pDescriptor->descriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets");


    int loop = 0;
    for (int i = 0; i < info.descriptorSetCount; ++i) {
        std::vector<VkWriteDescriptorSet> setWrites;
        std::vector<VkDescriptorBufferInfo> bufferInfos(*info.pDescriptorSplitCount);
        for (int index = 0; index < *info.pDescriptorSplitCount; ++index) {
            // Buffer info and data offset info
            // VIEW PROJECTION DESCRIPTOR
            bufferInfos[index].buffer = pDescriptor->buffer[loop];                       // Buffer to get data from
            bufferInfos[index].offset = 0;                                               // Offset into the data
            bufferInfos[index].range = info.dataSizes[loop];                             // Size of the data that is going to be bound to the descriptor set

            // Data about connection between binding and buffer
            VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = pDescriptor->descriptorSets[i];            // Descriptor set to update
            writeDescriptorSet.dstBinding = *info.pBindings;                       // Binding to update (matches with binding on layout/shader
            writeDescriptorSet.dstArrayElement = 0;                                // Index in the array we want to update
            writeDescriptorSet.descriptorType = *info.pDescriptorType;             // Type of descriptor we are updating
            writeDescriptorSet.descriptorCount = 1;                                // Amount to update
            writeDescriptorSet.pBufferInfo = &bufferInfos[index];                          // Information about buffer data to bind
            setWrites.push_back(writeDescriptorSet);
            info.pBindings++;
            info.pDescriptorType++;
            ++loop;
        }

        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(setWrites.size()),
                               setWrites.data(), 0, nullptr);

        info.pDescriptorSplitCount++;
    }

}

void Descriptors::cleanUp(ArDescriptor arDescriptor) {

    for (int i = 0; i < arDescriptor.buffer.size(); ++i) {
        vkFreeMemory(device, arDescriptor.bufferMemory[i], nullptr);
        vkDestroyBuffer(device, arDescriptor.buffer[i], nullptr);

    }

    for (int i = 0; i < arDescriptor.descriptorSetLayoutCount; ++i) {
        vkDestroyDescriptorSetLayout(device, arDescriptor.pDescriptorSetLayouts[i], nullptr);
    }


    vkDestroyDescriptorPool(device, arDescriptor.pDescriptorPool, nullptr);


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

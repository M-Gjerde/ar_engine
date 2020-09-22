//
// Created by magnus on 9/20/20.
//

#include <stdexcept>
#include <cstring>
#include "TextureLoading.h"
#include "Utils.h"


TextureLoading::TextureLoading() = default;


stbi_uc *TextureLoading::loadTextureFile(const std::string& fileName, int *width, int *height, VkDeviceSize *imageSize) {
    // Number of channels image uses
    int channels;

    // Load pixel data for image
    std::string fileLoc = "../textures/" + fileName;
    stbi_uc *image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);
    const char *const res = stbi_failure_reason();
    if (!image)
        throw std::runtime_error("Failed to load a Texture file (" + fileName + ")");

    // Calculate image size using: dimmensions * channels
    *imageSize = *width * *height * 4;

    return image;
}


void TextureLoading::createTextureSampler(Utils::MainDevice mainDevice, VkAllocationCallbacks allocator) {
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;                     // How to render when image is magnified on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;                     // How to render when image is minified on screen
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in W (z) direction, 3D textures
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;   // Set border color to black (If we have enabled border) Only works on border clamp
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;               // Whether coords should be normalized between or not
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;       // Mipmap interpolation mode
    samplerCreateInfo.mipLodBias = 0.0f;                                // Mipmap level bias (Level of detail)
    samplerCreateInfo.minLod = 0.0f;                                    // Minimum level of detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;                                    // Maximum level of Detail to pick mip level
    samplerCreateInfo.anisotropyEnable = VK_TRUE;                       // Enable Anisotropy
    samplerCreateInfo.maxAnisotropy = 16;                               // Anisotropy sample level

    VkResult result = vkCreateSampler(mainDevice.logicalDevice, &samplerCreateInfo, &allocator, &textureSampler);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a texture sampler");


}

void TextureLoading::getSampler(Utils::MainDevice mainDevice, VkSampler *pSampler, VkAllocationCallbacks allocator) {
    createTextureSampler(mainDevice, allocator);
    *pSampler = textureSampler;
}



int TextureLoading::getTextureImageLoc(Utils::MainDevice mainDevice, std::string fileName, VkQueue graphicsQueue,
                                       VkCommandPool graphicsCommandPool, std::vector<VkImage> *textureImages,
                                       std::vector<VkDeviceMemory> *textureImageMemory) {
    //Load in image file
    int width, height;
    VkDeviceSize imageSize;
    stbi_uc *imageData = loadTextureFile(fileName, &width, &height, &imageSize);

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer imageStagingBuffer;
    VkDeviceMemory imageStagingBufferMemory;
    Utils::createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageStagingBuffer,
                        &imageStagingBufferMemory);

    // Copy image data to staging buffer
    void *data;
    vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);

    //Free original image data memory
    stbi_image_free(imageData);

    // Create image to hold final texture
    VkImage texImage;
    VkDeviceMemory texImageMemory;
    texImage = Utils::createImage(mainDevice, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

    // COPY DATA TO IMAGE
    // Transition image to be DST for copy operation
    Utils::transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage,

                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //Copy image data
    Utils::copyImageBuffer(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width,
                           height);
    // Transition image to be shader readable for shader usage
    Utils::transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // Add texture data to vector for reference
    textureImages->push_back(texImage);
    textureImageMemory->push_back(texImageMemory);

    // Destroy staging buffers
    vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

    //Return texture image ID
    return textureImages->size() - 1;
}

int TextureLoading::createTextureDescriptor(Utils::MainDevice mainDevice, VkImageView textureImage,
                                            VkDescriptorPool samplerDescriptorPool,
                                            VkDescriptorSetLayout samplerSetLayout,
                                            std::vector<VkDescriptorSet> *samplerDescriptorSets) {
    VkDescriptorSet descriptorSet;

    // Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = samplerDescriptorPool;
    setAllocateInfo.descriptorSetCount = 1;
    setAllocateInfo.pSetLayouts = &samplerSetLayout;

    // Allocate Descriptor sets
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocateInfo, &descriptorSet);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate Texture Descriptor Set");

    // Texture Image info
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;           // Image layout when in use
    imageInfo.imageView = textureImage;                                         // Image to bind to se
    imageInfo.sampler = textureSampler;                                         // Sampler to use for set

    // Descriptor Write info
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;                                 // New set so we are starting from 0
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    // Update new descriptor set
    vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

    // Add descriptor set to list
    samplerDescriptorSets->push_back(descriptorSet);

    // return descriptor set location
    return samplerDescriptorSets->size() - 1;
}


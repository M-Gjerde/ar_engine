//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_TEXTURELOADING_H
#define AR_ENGINE_TEXTURELOADING_H


#include <vulkan/vulkan.h>
#include <vector>
#include "../include/stb_image.h"
#include "Utils.h"

class TextureLoading {
public:
    TextureLoading();

    void getSampler(Utils::MainDevice mainDevice, VkSampler *pSampler, VkAllocationCallbacks allocator);
    static int getTextureImageLoc(Utils::MainDevice mainDevice, std::string fileName, VkQueue graphicsQueue,
                           VkCommandPool graphicsCommandPool, std::vector<VkImage> *textureImages,
                           std::vector<VkDeviceMemory> *textureImageMemory);

    int createTextureDescriptor(Utils::MainDevice mainDevice, VkImageView textureImage,
                                       VkDescriptorPool samplerDescriptorPool, VkDescriptorSetLayout samplerSetLayout,
                                       std::vector<VkDescriptorSet> *samplerDescriptorSets);

private:

    // - Vulkan components
    VkSampler textureSampler{};


    // - Create functions
    void createTextureSampler(Utils::MainDevice mainDevice, VkAllocationCallbacks allocator);
    static stbi_uc * loadTextureFile(const std::string& fileName, int* width, int* height, VkDeviceSize *imageSize);
};


#endif //AR_ENGINE_TEXTURELOADING_H

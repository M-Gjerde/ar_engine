//
// Created by magnus on 10/13/20.
//


#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include "Textures.h"

Textures::Textures(Images *pImages) : Images(pImages){
    images = pImages;
}

void Textures::createTextureSampler(){

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(images->mainDevice.device, &samplerInfo, nullptr, &arTextureSampler.textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }


}
void Textures::createTextureImage(std::string fileName) {

    std::string filePath = "../textures/" + fileName + ".jpg";

    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //TODO METHOD
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    imageBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    imageBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    imageBuffer.bufferSize = imageSize;

    images->createBuffer(&imageBuffer);

    // Create image to bind memory
    images->createImage(texWidth, texHeight, VK_FORMAT_B8G8R8A8_SRGB,
                        VK_IMAGE_TILING_LINEAR,
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        arTextureSampler.textureImage,
                        arTextureSampler.textureImageMemory,
                        VK_IMAGE_LAYOUT_PREINITIALIZED);

    // Copy pixel data to image memory
    void* data;
    vkMapMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(images->mainDevice.device, arTextureSampler.textureImageMemory);
    stbi_image_free(pixels);


       transitionImageLayout(arTextureSampler.textureImage,
                             VK_FORMAT_B8G8R8A8_SRGB,
                             VK_IMAGE_LAYOUT_PREINITIALIZED,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             arTextureSampler.transferCommandPool,
                             arTextureSampler.transferQueue
                             );
/*
       copyBufferToImage(imageBuffer.buffer, arTextureSampler.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
                         arTextureSampler.transferCommandPool,
                         arTextureSampler.transferQueue);
       transitionImageLayout(arTextureSampler.textureImage, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             arTextureSampler.transferCommandPool,
                             arTextureSampler.transferQueue);
   */
    vkFreeMemory(images->mainDevice.device, imageBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(images->mainDevice.device, imageBuffer.buffer, nullptr);

}

void Textures::createTextureImageView(){

    arTextureSampler.textureImageView = images->createImageView(arTextureSampler.textureImage, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

}


void Textures::cleanUp() {
    vkDestroySampler(images->mainDevice.device, arTextureSampler.textureSampler, nullptr);

    vkDestroyImageView(images->mainDevice.device, arTextureSampler.textureImageView, nullptr);

    vkDestroyImage(images->mainDevice.device, arTextureSampler.textureImage, nullptr);
    vkFreeMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, nullptr);
}

void Textures::createTexture(ArTextureSampler *pArTextureSampler) {
    arTextureSampler = *pArTextureSampler ;
    createTextureImage("landscape");
    createTextureImageView();
    createTextureSampler();


    *pArTextureSampler = arTextureSampler;
}

void Textures::updateTexture(std::string fileName){
    std::string filePath = "../textures/" + fileName + ".jpg";

    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //TODO METHOD
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // Copy pixel data to image memory
    void* data;
    vkMapMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(images->mainDevice.device, arTextureSampler.textureImageMemory);
    stbi_image_free(pixels);


}

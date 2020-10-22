//
// Created by magnus on 10/13/20.
//


#define STB_IMAGE_IMPLEMENTATION

#include "../include/stb_image.h"
#include "Textures.h"

#include <utility>
#include <iostream>
Textures::Textures(Images *pImages) : Images(pImages) {
    images = pImages;
}

void Textures::createTextureSampler() {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(images->mainDevice.device, &samplerInfo, nullptr, &arTextureSampler.textureSampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }


}

void Textures::createTextureImage(std::string fileName, Disparity *disparity) {


    int texWidth, texHeight, texChannels;
    stbi_uc *pixels;
    std::string filePath = "../textures/" + fileName;
    if (fileName == "landscape.jpg" || fileName == "wallpaper.png" || fileName == "output-onlinepngtools.jpg" ) {
        pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //TODO METHOD
        texChannels = 4; // 1+ beacuse of alpha
        format = VK_FORMAT_R8G8B8A8_SRGB;
    } else {
        pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //TODO METHOD
        texChannels = 4; // 1+ beacuse of alpha
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    VkDeviceSize imageSize = texWidth * texHeight * texChannels;

    if (fileName == "cvtThreeChannel.png"){

        disparity->getDisparityFromImage(pixels);
        imageSize = disparity->imageSize;
        format = VK_FORMAT_R8G8B8A8_SRGB;
        texChannels = 4;
        texWidth = disparity->imageWidth;
        texHeight = disparity->imageHeight;
    }

    if (!pixels) {
        std::cout << "Something went wrong\n";
        throw std::runtime_error("failed to load texture image: " + fileName);
    }



  /*  imageBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    imageBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    imageBuffer.bufferSize = imageSize;

    images->createBuffer(&imageBuffer);
*/
    // Create image to bind memory
    images->createImage(texWidth, texHeight, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        arTextureSampler.textureImage, arTextureSampler.textureImageMemory, VK_IMAGE_LAYOUT_UNDEFINED);



    transitionImageLayout(arTextureSampler.textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                          arTextureSampler.transferCommandPool, arTextureSampler.transferQueue);


    void *data;

    vkMapMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));


    stbi_image_free(pixels);


    transitionImageLayout(arTextureSampler.textureImage, format, VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arTextureSampler.transferCommandPool,
                          arTextureSampler.transferQueue);

    /*
    copyBufferToImage(imageBuffer.buffer, arTextureSampler.textureImage, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight), arTextureSampler.transferCommandPool,
                      arTextureSampler.transferQueue);

    transitionImageLayout(arTextureSampler.textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arTextureSampler.transferCommandPool,
                          arTextureSampler.transferQueue);

    vkFreeMemory(images->mainDevice.device, imageBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(images->mainDevice.device, imageBuffer.buffer, nullptr);
*/
}

void Textures::createTextureImageView() {

    arTextureSampler.textureImageView = images->createImageView(arTextureSampler.textureImage, format,
                                                                VK_IMAGE_ASPECT_COLOR_BIT);

}


void Textures::cleanUp() {
    vkDestroySampler(images->mainDevice.device, arTextureSampler.textureSampler, nullptr);

    vkDestroyImageView(images->mainDevice.device, arTextureSampler.textureImageView, nullptr);

    vkDestroyImage(images->mainDevice.device, arTextureSampler.textureImage, nullptr);
    vkFreeMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, nullptr);
}

void Textures::createTexture(ArTextureSampler *pArTextureSampler, std::string fileName, Disparity *disparity) {
    arTextureSampler = *pArTextureSampler;
    createTextureImage(std::move(fileName), disparity);
    createTextureImageView();
    createTextureSampler();


    *pArTextureSampler = arTextureSampler;
}


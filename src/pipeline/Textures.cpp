//
// Created by magnus on 10/13/20.
//


#define STB_IMAGE_IMPLEMENTATION

#include "../include/stb_image.h"
#include "Textures.h"

#include <utility>
#include <iostream>
#include <zconf.h>

Textures::Textures(Images *pImages) : Images(pImages) {
    images = pImages;
}

void Textures::createTextureSampler(ArTextureImage *arTextureSampler) {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(images->mainDevice.device, &samplerInfo, nullptr, &arTextureSampler->textureSampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }


}

void Textures::createTextureImage(std::string fileName, ArTextureImage *arTextureSampler, ArBuffer *imageBuffer) {

    // Load image using stb_image.h
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels;
    std::string filePath = "../textures/" + fileName;

    pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //TODO METHOD
    texChannels = 4; // 1+ beacuse of alpha
    format = VK_FORMAT_R8G8B8A8_UNORM;

    if (!pixels) throw std::runtime_error("failed to load texture image: " + fileName);

    // Creating staging buffer
    imageBuffer->bufferSize = texWidth * texHeight * texChannels;
    images->createBuffer(imageBuffer);

    // Copy data to buffer
    void *data;
    vkMapMemory(images->mainDevice.device, imageBuffer->bufferMemory, 0, imageBuffer->bufferSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageBuffer->bufferSize));
    vkUnmapMemory(images->mainDevice.device, imageBuffer->bufferMemory);

    // Create image and bind image memory
    images->createImage(texWidth, texHeight, format, VK_IMAGE_TILING_LINEAR,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, arTextureSampler->textureImage,
                        arTextureSampler->textureImageMemory, VK_IMAGE_LAYOUT_UNDEFINED);

    // Transition image to transfer destination
    transitionImageLayout(arTextureSampler->textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arTextureSampler->transferCommandPool,
                          arTextureSampler->transferQueue);

    // Copy data from buffer to image
    copyBufferToImage(imageBuffer->buffer, arTextureSampler->textureImage, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight), arTextureSampler->transferCommandPool,
                      arTextureSampler->transferQueue);

    // Transition image to present format
    transitionImageLayout(arTextureSampler->textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arTextureSampler->transferCommandPool,
                          arTextureSampler->transferQueue);


}

void Textures::createTextureImageView(ArTextureImage *arTextureSampler) {
    arTextureSampler->textureImageView = images->createImageView(arTextureSampler->textureImage, format,
                                                                 VK_IMAGE_ASPECT_COLOR_BIT);
}


void Textures::cleanUp(ArTextureImage arTextureSampler, ArBuffer imageBuffer) {
    // Free buffer memory
    vkFreeMemory(images->mainDevice.device, imageBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(images->mainDevice.device, imageBuffer.buffer, nullptr);

    vkDestroySampler(images->mainDevice.device, arTextureSampler.textureSampler, nullptr);

    vkDestroyImageView(images->mainDevice.device, arTextureSampler.textureImageView, nullptr);

    vkDestroyImage(images->mainDevice.device, arTextureSampler.textureImage, nullptr);
    vkFreeMemory(images->mainDevice.device, arTextureSampler.textureImageMemory, nullptr);
}

void Textures::createTexture(std::string fileName, ArTextureImage *pArTextureSampler, ArBuffer *imageStagingBuffer) {

    createTextureImage(std::move(fileName), pArTextureSampler, imageStagingBuffer);
    createTextureImageView(pArTextureSampler);
    createTextureSampler(pArTextureSampler);

}

void Textures::createTextureImage(ArTextureImage *arTexture, ArBuffer *textureBuffer) {
    format = VK_FORMAT_R8_UNORM;
    int imageSize = arTexture->height * arTexture->width * arTexture->channels;
    // Create image and bind image memory
    images->createImage(arTexture->width, arTexture->height, format, VK_IMAGE_TILING_LINEAR,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, arTexture->textureImage, arTexture->textureImageMemory,
                        VK_IMAGE_LAYOUT_UNDEFINED);

    // Creating staging buffer
    textureBuffer->bufferSize = imageSize;
    images->createBuffer(textureBuffer);

    arTexture->textureImageView = images->createImageView(arTexture->textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

    createTextureSampler(arTexture);

    // Map texture image memory to void pointer
    vkMapMemory(images->mainDevice.device, textureBuffer->bufferMemory, 0, textureBuffer->bufferSize, 0,
                &arTexture->data);


}

void Textures::setDisparityImageTexture(ArTextureImage *arTextureSampler, ArBuffer *imageBuffer) {


    // Load texture from disparity
    int width, height;
    auto *pixels = new unsigned char[imageBuffer->bufferSize];
    //disparity->getDisparityFromImage(&pixels);
    //width = disparity->imageWidth;
    //height = disparity->imageHeight;
    format = VK_FORMAT_R8_UNORM;

    // Copy data to buffer
    void *data;
    vkMapMemory(images->mainDevice.device, imageBuffer->bufferMemory, 0, imageBuffer->bufferSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageBuffer->bufferSize));
    vkUnmapMemory(images->mainDevice.device, imageBuffer->bufferMemory);



    // Transition image to transfer destination
    transitionImageLayout(arTextureSampler->textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arTextureSampler->transferCommandPool,
                          arTextureSampler->transferQueue);

    // Copy data from buffer to image
    copyBufferToImage(imageBuffer->buffer, arTextureSampler->textureImage, static_cast<uint32_t>(width),
                      static_cast<uint32_t>(height), arTextureSampler->transferCommandPool,
                      arTextureSampler->transferQueue);

    // Transition image to present format
    transitionImageLayout(arTextureSampler->textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arTextureSampler->transferCommandPool,
                          arTextureSampler->transferQueue);

}

void Textures::setDisparityVideoTexture(ArTextureImage *videoTexture, ArBuffer *imageBuffer) {
    format = VK_FORMAT_R8_UNORM;


    // Copy data to buffer note pixelData is located in Disparity handle
    //memcpy(videoTexture->data, disparity->pixelData, static_cast<size_t>(imageBuffer->bufferSize));


    // Transition image to transfer destination
    transitionImageLayout(videoTexture->textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, videoTexture->transferCommandPool,
                          videoTexture->transferQueue);

    // Copy data from buffer to image
    copyBufferToImage(imageBuffer->buffer, videoTexture->textureImage, static_cast<uint32_t>(videoTexture->width),
                      static_cast<uint32_t>(videoTexture->height), videoTexture->transferCommandPool,
                      videoTexture->transferQueue);

    // Transition image to present format
    transitionImageLayout(videoTexture->textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, videoTexture->transferCommandPool,
                          videoTexture->transferQueue);


}


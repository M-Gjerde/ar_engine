//
// Created by magnus on 10/13/20.
//

#ifndef AR_ENGINE_IMAGES_H
#define AR_ENGINE_IMAGES_H


#include <vulkan/vulkan.h>
#include <vector>
#include "../include/structs.h"
#include "Buffer.h"

class Images : Buffer{

public:
    Images(MainDevice newMainDevice, VkExtent2D swapchainExtent);

    void createDepthImageView(VkImageView *depthImageView);
    VkFormat findDepthFormat();

    void cleanUp();

private:
    MainDevice mainDevice;
    ArDepthResource arDepthResource;

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
};



#endif //AR_ENGINE_IMAGES_H

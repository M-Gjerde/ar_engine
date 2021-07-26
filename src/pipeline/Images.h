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
    MainDevice mainDevice{};


    Images(MainDevice newMainDevice, VkExtent2D swapchainExtent);
    explicit Images(Images* pImages);
    void createDepthImageView(VkImageView *depthImageView);
    VkFormat findDepthFormat();

    virtual void cleanUp();
    void createBuffer(ArBuffer* pArBuffer) override;

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkImageLayout initialLayout, VkMemoryPropertyFlags properties, VkImage &image,
                     VkDeviceMemory &imageMemory, VkMemoryRequirements *memRequirements);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

    void
    transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool,
                          VkQueue transferQueue);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandPool commandPool, VkQueue transferQueue);

private:
    ArDepthResource arDepthResource{};

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


};



#endif //AR_ENGINE_IMAGES_H

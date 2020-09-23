//
// Created by magnus on 9/20/20.
//

#include <stdexcept>
#include <fstream>
#include <iostream>
#include "Utils.h"

Utils::Utils() = default;

Utils::~Utils() = default;

void
Utils::copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer,
                       VkImage image, uint32_t width, uint32_t height) {

    VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

    VkBufferImageCopy imageRegion = {};
    imageRegion.bufferOffset = 0;                                           // Offset into data
    imageRegion.bufferRowLength = 0;                                        // For calculating data spacing. Row length of data to calculate data spacing
    imageRegion.bufferImageHeight = 0;                                      // Image height to calculate data spacing
    imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Which aspect of image to copy
    imageRegion.imageSubresource.mipLevel = 0;                              // Mipmap level to copy
    imageRegion.imageSubresource.baseArrayLayer = 0;                        // Starting array layer (if array)
    imageRegion.imageSubresource.layerCount = 1;                            // How many layers we want to copy accros
    imageRegion.imageOffset = {0, 0,
                               0};                          // Start at the origin and copy everything from there. (offset into image as opposed to raw data in buffer offset)
    imageRegion.imageExtent.depth = 1;
    imageRegion.imageExtent.height = height;
    imageRegion.imageExtent.width = width;

    // Copy buffer to given image
    vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &imageRegion);


    endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);

}

VkCommandBuffer Utils::beginCommandBuffer(VkDevice device, VkCommandPool commandPool) {
    // Command buffer to hold transfer commands
    VkCommandBuffer commandBuffer;
    // Command buffer details
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

    // Information to begin the command buffer record
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;      // We're only using the command buffer once, so set up for one time submit

    // Begin recording transfer commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Utils::endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                                      VkCommandBuffer commandBuffer) {



    // End commands
    vkEndCommandBuffer(commandBuffer);

    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    // Wait until this queue is done, then continue Not optimal for thousands of meshes
    vkQueueWaitIdle(queue);
    // Free temporary command buffer back to pool
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Utils::createBuffer(MainDevice mainDevice, VkDeviceSize bufferSize,
                         VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer *buffer,
                         VkDeviceMemory *bufferMemory) {

    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;                                   // Size of buffer (size of 1 vertex * number of vertices) Size in memory
    bufferInfo.usage = bufferUsage;           // Types of buffer, however we want vertex buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;             // Similar to swap chain images, can share vertex buffers

    // Create buffer
    VkResult result = vkCreateBuffer(mainDevice.logicalDevice, &bufferInfo, nullptr, buffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(mainDevice.logicalDevice, *buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice,
                                                             memRequirements.memoryTypeBits,              //Index of memory type on physical device that has required bit flags
                                                             bufferProperties);                        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : CPU can interact without cache capabilities

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(mainDevice.logicalDevice, *buffer, *bufferMemory, 0);


}

uint32_t
Utils::findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties) {
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((allowedTypes & (i << i)) &&                                             // Index of memory type must match
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i; // This memory type is valid, so return index.
        }
    }
    return -1;
}

std::vector<char> Utils::readFile(const std::string &filename) {
    //  Open stream from given file
    //  Reads it in binary and starts reading from end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    //  Check if file stream successfully opened
    if (!file.is_open())
        throw std::runtime_error("Failed to open a file");

    //  Get current read position and use to resize file buffer
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> fileBuffer(fileSize);

    //  Move read position (seek to) to the start of the file and read the file
    file.seekg(0);
    file.read(fileBuffer.data(), fileSize);
    file.close();
    return fileBuffer;
}

void Utils::copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer,
                       VkBuffer dstBuffer, VkDeviceSize bufferSize) {

    // Create buffer
    VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    // Command to copy src buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

    endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);

}

void Utils::transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image,
                                  VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;                                    // Layout to transition from
    imageMemoryBarrier.newLayout = newLayout;                                    // Layout to transition to
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;            // Queue Family to transition from
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;            // Queue Family to transition to
    imageMemoryBarrier.image = image;                                            // Image being accessed and modified as part of barrier
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Aspect of image being altered
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;                        // First mip level to start alteration on
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;                      // Number of mip levels to alter starting form base
    imageMemoryBarrier.subresourceRange.layerCount = 1;                          // First layer to start alterations on
    imageMemoryBarrier.subresourceRange.levelCount = 1;                          // Number of layers to alter starting from baseArrayLayer

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = 0;                                        // Starting from any point in time we want a transfer from oldLayout to newLayout to transfer before dstAccessMask
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;             // The destination

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    }
        // IF transitioning from transfer destination to shader available
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,                                            // Pipeline stages (match to src and dst AccessMaks
            0,                                                        // Dependency flags
            0, nullptr,                              // Memory barrier count + data
            0, nullptr,                     // Buffer memory barrier count + data
            1, &imageMemoryBarrier                             // Image memory barrier count + data
    );

    endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}

VkImage
Utils::createImage(MainDevice mainDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags,
                   VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory) {
    // CREATE IMAGE
    //Image creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                             // Type of image (1D, 2D or 3D)
    imageCreateInfo.extent.height = height;                                   // Height of image extent
    imageCreateInfo.extent.width = width;                                     // Width of image extent
    imageCreateInfo.extent.depth = 1;                                         // depth of image extent  (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;                                            // Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;                                          // Number of levels in image array;
    imageCreateInfo.format = format;                                          // Format type of image
    imageCreateInfo.tiling = tiling;                                          // How image data should be "tiled" (arranged for optimal reading speed)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                // Layout of iamge data on creation
    imageCreateInfo.usage = useFlags;                                         // Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                          // Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                  // Sharing mode between queues.

    VkImage image;
    VkResult result = vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an Image");

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = Utils::findMemoryTypeIndex(mainDevice.physicalDevice,
                                                                    memoryRequirements.memoryTypeBits, propFlags);
    result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocateInfo, nullptr, imageMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate memory for image");

    // Connect memory to image, in imagememory we could have data for 10 images therefore we have a offset
    vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0);

    return image;
}

VkImageView Utils::createImageView(MainDevice mainDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;                                           //Image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                        //Working with 2D images (CUBE flag is usefull for skyboxes)
    viewCreateInfo.format = format;                                         //Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;            //Allows re-mapping rgba components of rba values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    //  SubResources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;               //Which aspect of image to view (e.g. COLOR_BIT for viewing color)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;                       //Start mimap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;                         //Number of mimap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;                     //Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;                         //Number of array levels to view

    //Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an image view");
    return imageView;
}

VkFormat
Utils::chooseSupportedFormat(Utils::MainDevice mainDevice, const std::vector<VkFormat> &formats, VkImageTiling tiling,
                             VkFormatFeatureFlags featureFlags) {
    // Loop through options and find compatible ones
    for (VkFormat format : formats) {
        // Get properties for given format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(mainDevice.physicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
            return format;
    }

    throw std::runtime_error("Failed to find a matching format");
}



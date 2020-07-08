//
// Created by magnus on 7/5/20.
//

#ifndef UDEMY_VULCAN_UTILITIES_H
#define UDEMY_VULCAN_UTILITIES_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <fstream>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 2;

const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


// Vertex data representation
struct Vertex {
    glm::vec3 pos; // Vertex Position (x, y, z)
    glm::vec3 col; // Vertex color (r, g, b)
};

//  Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
    int graphicsFamily = -1;                //Location of graphics queue family
    int presentationFamily = -1;            //Location of Presentation Queue Family

    [[nodiscard]] bool isValid() const {
        return graphicsFamily >= 0 && presentationFamily >= 0;
    }

};

struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;       //Surface properties, e.g. image size/extent
    std::vector<VkSurfaceFormatKHR> formats;            //Surface image formats
    std::vector<VkPresentModeKHR> presentationModes;    //How our images are presented onto the screen
};


struct SwapchainImage {
    VkImage image;
    VkImageView imageView;
};

static std::vector<char> readFile(const std::string &filename) {
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


static uint32_t
findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties) {
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((allowedTypes & (i << i)) &&                                             // Index of memory type must match
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i; // This memory type is valid, so return index.
        }
    }

}

static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize,
                         VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer *buffer,
                         VkDeviceMemory *bufferMemory) {


    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;                                   // Size of buffer (size of 1 vertex * number of vertices) Size in memory
    bufferInfo.usage = bufferUsage;           // Types of buffer, however we want vertex buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;             // Similar to swap chain images, can share vertex buffers

    // Create buffer
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(
            physicalDevice,
            memRequirements.memoryTypeBits,              //Index of memory type on physical device that has required bit flags
            bufferProperties);                        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : CPU can interact without cache capabilities

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, bufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);

}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
    // Command buffer to hold transfer commands
    VkCommandBuffer transferCommandBuffer;
    // Command buffer details
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = transferCommandPool;
    allocateInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    vkAllocateCommandBuffers(device, &allocateInfo, &transferCommandBuffer);

    // Information to begin the command buffer record
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;      // We're only using the command buffer once, so set up for one time submit

    // Begin recording transfer commands
    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    // Command to copy src buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

    // End commands
    vkEndCommandBuffer(transferCommandBuffer);

    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    // Submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // Wait until this queue is done, then continue Not optimal for thousands of meshes
    vkQueueWaitIdle(transferQueue);
    // Free temporary command buffer back to pool
    vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
}


#endif //UDEMY_VULCAN_UTILITIES_H

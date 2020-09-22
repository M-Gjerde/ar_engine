//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_UTILS_H
#define AR_ENGINE_UTILS_H


#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class Utils {

public:
    Utils();
    ~Utils();

    static const int MAX_FRAME_DRAWS = 2;
    static const int MAX_OBJECTS = 2;

    static inline const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


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

    struct MainDevice {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    };


    static VkFormat
    chooseSupportedFormat(Utils::MainDevice mainDevice, const std::vector<VkFormat> &formats, VkImageTiling tiling,
                          VkFormatFeatureFlags featureFlags);

    static void
    copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer,
                    VkImage image, uint32_t width, uint32_t height);


    static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize,
                             VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer *buffer,
                             VkDeviceMemory *bufferMemory);

    static uint32_t
    findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);


    static std::vector<char> readFile(const std::string &filename);


    static void
    copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer,
               VkBuffer dstBuffer, VkDeviceSize bufferSize);

    static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image,
                                      VkImageLayout oldLayout, VkImageLayout newLayout);

    static VkImage createImage(MainDevice deviceHandle, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);

    static VkImageView createImageView(MainDevice mainDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
private:



    static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);

    static void
    endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);


};


#endif //AR_ENGINE_UTILS_H

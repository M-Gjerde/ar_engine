//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_PLATFORM_H
#define AR_ENGINE_PLATFORM_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include "../include/structs.h"
#include "../include/Validation.h"

struct {
    int WIDTH = 600;
    int HEIGHT = 480;
} viewport;

class Platform {

public:

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    Platform(GLFWwindow *window, ArEngine *arEngine);
    void createCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    ~Platform();

    void cleanUp();

private:

    // Vulkan components
    ArEngine arEngine;

    // - Extensions
    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Create functions
    void createInstance();
    void createSurface(GLFWwindow *window);
    void setDebugMessenger();
    void createSwapchain();
    void createLogicalDevice();
    void createSwapchainImageViews();

    // Helper functions
    [[nodiscard]] VkPhysicalDevice_T * selectPhysicalDevice() const;
    bool hasDeviceExtensionsSupport();
    void pickPhysicalDevice();

    // - Swapchain support
    SwapChainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

};


#endif //AR_ENGINE_PLATFORM_H

//
// Created by magnus on 8/28/21.
//

#ifndef AR_ENGINE_VULKANRENDERER_H
#define AR_ENGINE_VULKANRENDERER_H


#include <string>

class VulkanRenderer {

public:

    VulkanRenderer(bool enableValidation = false);
    virtual ~VulkanRenderer();
    /** @brief Setup the vulkan instance, enable required extensions and connect to the physical device (GPU) */
    bool initVulkan();


    /** @brief Example settings that can be changed by ... */
    struct Settings {
        /** @brief Activates validation layers (and message output) when set to true */
        bool validation = false;
        /** @brief Set to true if fullscreen mode has been requested via command line */
        bool fullscreen = false;
        /** @brief Set to true if v-sync will be forced for the swapchain */
        bool vsync = false;
        /** @brief Enable UI overlay */
        bool overlay = true;
    } settings;

    std::string title = "Vulkan Renderer";
    std::string name = "VulkanRenderer";
    uint32_t apiVersion = VK_API_VERSION_1_0;

protected:
    // Vulkan Instance, stores al per-application states
    VkInstance instance;
    // Physical Device that Vulkan will use
    VkDevice physicalDevice;
    //Physical device properties (device limits etc..)
    VkPhysicalDeviceProperties deviceProperties;
    // Features available on the physical device
    VkPhysicalDeviceFeatures deviceFeatures;
    // Features all available memory types for the physical device
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    /** @brief Logical device, application's view of the physical device (GPU) */
    VkDevice device;
    // Handle to the device graphics queue that command buffers are submitted to
    VkQueue queue;

    // Handle to Debug Utils
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
private:


    VkResult createInstance(bool enableValidation);

};





#endif //AR_ENGINE_VULKANRENDERER_H

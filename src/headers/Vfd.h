//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_VFD_H
#define AR_ENGINE_VFD_H

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "../source/Allocator.h"
#include "../include/utilities.h"
#include "../libs/Utils.h"

#include <iostream>
#include <vector>
#include <cstring>
#include <set>

class Vfd {
public:
    Vfd();

    ~Vfd();

    int init(GLFWwindow *newWindow);
    void cleanUp();

    // - Getters
    void getInstance(VkInstance* pInstance);
    void getSurface(VkSurfaceKHR* VkSurface);
    void getSelectedPhysicalDevice(VkPhysicalDevice *pPhysicalDevice) const;
    void getSetDebugMessenger(VkDebugUtilsMessengerEXT *pDebugMessenger);
    void getLogicalDevice(VkDevice * pLogicalDevice) const;
    void getGraphicsQueue(VkQueue* pQueue);
    void getPresentationQueue(VkQueue* pQueue);
    void getSwapchain(VkSwapchainKHR* pSwapchain);
    void getSwapchainImages(std::vector<Utils::SwapchainImage> *pSwapchainImages);
    void getSwapchainImageFormat(VkFormat* pFormat);
    void getSwapchainExtent(VkExtent2D* pExtent2D);
    void getSwapchainFramebuffers(std::vector<VkFramebuffer>* pFrameBuffers);

    VkAllocationCallbacks getAllocator();
private:
    GLFWwindow* window{};

    // Vulkan components
    // - Main
    VkInstance instance{};
    VkQueue graphicsQueue{};
    VkQueue presentationQueue{};
    VkAllocationCallbacks vkAllocationCallbacks = {};
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkSurfaceKHR surface{};

    VkSwapchainKHR swapchain{};
    std::vector<Utils::SwapchainImage> swapChainImages;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    //  - Utility
    VkFormat  swapChainImageFormat{};
    VkExtent2D swapChainExtent{};




    struct MainDevice {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } mainDevice{};

    // - Create functions
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();

    // Helper functions
    bool checkValidationLayerSupport();
    static bool checkInstanceExtensionSupport(std::vector <const char*> *checkExtensions);
    bool checkDeviceSuitable(VkPhysicalDevice device);
    Utils::QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    Utils::SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);
    static VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
    static VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) ;


        //  - Validation Layers
    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };


    //  -- Debugger functions

    static VkResult
    CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                 const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, [[maybe_unused]] void *pUserData);

};


#endif //AR_ENGINE_VFD_H

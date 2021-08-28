//
// Created by magnus on 8/28/21.
//

#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>
#include "VulkanRenderer.h"
#include "Validation.h"

VulkanRenderer::VulkanRenderer(bool enableValidation) {

    settings.validation = enableValidation;

    initVulkan();

}

VkResult VulkanRenderer::createInstance(bool enableValidation) {
    settings.validation = enableValidation;


    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = name.c_str();
    appInfo.pEngineName = name.c_str();
    appInfo.apiVersion = apiVersion;

    // Get extensions supported by the instance
    std::vector<const char *> instanceExtension = Validation::getRequiredExtensions(settings.validation);

    // Check if extensions are supported
    if (!Validation::checkInstanceExtensionSupport(instanceExtension))
        throw std::runtime_error("Instance Extensions not supported");


    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    if (!instanceExtension.empty()){
        if (settings.validation){
            instanceExtension.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtension.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtension.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    if (settings.validation){
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

        // Check if this layer is available at instance level
        if (Validation::checkValidationLayerSupport(validationLayers)){
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        }
        else {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled\n";
        }

        return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);


    }


}

bool VulkanRenderer::initVulkan() {
    // Create Instance
    VkResult err = createInstance(settings.validation);
    if (err) {
        throw std::runtime_error("Could not create Vulkan instance");
    }
    // If requested, we enable the default validation layers for debugging
// If requested, we enable the default validation layers for debugging
    if (settings.validation)
    {
        // The report flags determine what type of messages for the layers will be displayed
        // For validating (debugging) an application the error and warning bits should suffice
        VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        // Additional flags include performance info, loader and layer debug messages, etc.
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Validation::populateDebugMessengerCreateInfo(createInfo);

        if (Validation::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugUtilsMessenger) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        //vks::debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
    }

    // Get list of devices and capabilities of each device

    // Select physical device to be used for the Vulkan example
    // Defaults to the first device unless anything else specified


    // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)

    // Vulkan device creation
    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device

    // Get a graphics queue from the device

    // Find a suitable depth format

    // Create synchronization objects



    return true;
}

VulkanRenderer::~VulkanRenderer() {
    // CleanUP all vulkan resources

    vkDestroyInstance(instance, nullptr);


}


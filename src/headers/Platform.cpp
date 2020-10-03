//
// Created by magnus on 10/3/20.
//

#include "Platform.h"


void Platform::setupDevice(GLFWwindow *window, VkInstance *instance, VkSurfaceKHR *surface, MainDevice mainDevice) {
    createInstance();
    setDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
}

void Platform::createInstance() {

    if (!Validation::checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    // Generic Vulkan app info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello SmartMirror";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "AR_ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;


    auto extensions = Validation::getRequiredExtensions();

    //Check instance extensions supported...
    if (!Validation::checkInstanceExtensionSupport(&extensions))
        throw std::runtime_error("vkInstance does not support required extensions");

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        Validation::populateDebugMessengerCreateInfo(debugCreateInfo);
        //createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");


}

void Platform::createSurface(GLFWwindow *window) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");

    }

}



void Platform::setDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    Validation::populateDebugMessengerCreateInfo(createInfo);

    if (Validation::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Platform::cleanUp() {
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if (enableValidationLayers) {
        Validation::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);

}

void Platform::findGPUDevices() {
    uint32_t deviceCount;

    VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    std::vector<VkPhysicalDeviceProperties> deviceProperties(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (int i = 0; i < deviceCount; ++i) {

        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties[i]);
        printf("Devices: %s\n", deviceProperties[i].deviceName);
    }


}

void Platform::pickPhysicalDevice() {
    findGPUDevices();
}

Platform::Platform() = default;

Platform::~Platform() = default;

//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_PLATFORM_H
#define AR_ENGINE_PLATFORM_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../include/structs.h"
#include "Validation.h"
class Platform {

public:
    Platform();

    void setupDevice(GLFWwindow *window, VkInstance *instance, VkSurfaceKHR *surface, MainDevice mainDevice);

    ~Platform();

    void cleanUp();

private:

    // Vulkan components
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkSurfaceKHR surface{};

    // Create functions
    void createInstance();
    void createSurface(GLFWwindow *window);
    void setDebugMessenger();

    // Helper functions
    void findGPUDevices();
    void pickPhysicalDevice();

};


#endif //AR_ENGINE_PLATFORM_H

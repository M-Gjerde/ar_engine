//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Platform.h"
#include "../include/structs.h"
#include <stdexcept>
#include <vector>
#include <iostream>

class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);


    void draw();
    void cleanup();

    ~VulkanRenderer();


private:
    GLFWwindow *glfWwindow{};

    // Vulkan components
    Platform extVk;
    VkInstance instance{};
    VkSurfaceKHR surface{};

    // Vulkan Device
    MainDevice mainDevice{};


};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP

//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "validation.h"

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
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debugMessenger{};

    void createInstance();
    void setDebugMessenger();


};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP

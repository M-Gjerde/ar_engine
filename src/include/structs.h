//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_STRUCTS_H
#define AR_ENGINE_STRUCTS_H


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct MainDevice {
    VkDevice device;
    VkPhysicalDevice physicalDevice;

};


#endif //AR_ENGINE_STRUCTS_H

//
// Created by magnus on 7/4/20.
//



#include "../headers/VulkanRenderer.hpp"

VulkanRenderer::~VulkanRenderer() = default;

VulkanRenderer::VulkanRenderer() = default;

int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform.setupDevice(newWindow, &instance, &surface, &mainDevice);


    } catch (std::runtime_error &err) {
        printf("Error: %s\n", err.what());
    }
    return 0;
}


void VulkanRenderer::draw() {

}

void VulkanRenderer::cleanup() {
    platform.cleanUp();

}

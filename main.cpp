#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "src/headers/VulkanRenderer.hpp"

struct AllocationMetrics {
    uint32_t TotalAllocated = 0;
    uint32_t TotalFreed = 0;

    [[nodiscard]] unsigned int currentUsage() const {
        return TotalAllocated - TotalFreed;
    }
};
static AllocationMetrics s_AllocationMetrics;
void *operator new(size_t size) {
    s_AllocationMetrics.TotalAllocated += size;
    return malloc(size);
}
void operator delete(void *memory, size_t size) {
    s_AllocationMetrics.TotalFreed += size;
    return free(memory);
}
struct Object {
    int x, y, z;
};
static void printMemoryUsage() {
    std::cout << "Memory Usage: " << s_AllocationMetrics.currentUsage() << " bytes" << std::endl;
}

GLFWwindow *window;
VulkanRenderer vulkanRenderer;

void initWindow(const std::string &wName = "Test Window", const int width = 800, const int height = 600) {
    glfwInit();
    // SET GLFW to NOT work with OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() {
    initWindow("Vulkan", 1200, 700);

    //Create VulkanRenderer instance
    if (vulkanRenderer.init(window) == EXIT_FAILURE)
        return EXIT_FAILURE;

    //Rotation
    float angle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime = 0.0f;


    int helicopter = vulkanRenderer.createMeshModel("../objects/uh60.obj");

    //Loop until close
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;
        if (angle > 360) angle -= 360.0f;


        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        vulkanRenderer.updateModel(helicopter, testMat);

        vulkanRenderer.draw();
    }

    vulkanRenderer.cleanup();

    //Destroy window and stop GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

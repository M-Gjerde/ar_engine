#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "src/headers/VulkanRenderer.hpp"
#include "src/headers/PlayerController.h"
#include "src/headers/InitiateVulkan.h"
#include <zconf.h>

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
PlayerController playerController;
float forward = 0;

void key_callback(GLFWwindow* glfWwindow, int key, int scancode, int action, int mods)
{

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwTerminate();
    }
    if (key == GLFW_KEY_W){
        forward++;
        playerController.uboViewProjection.view = glm::lookAt(glm::vec3(forward, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -5.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));

        vulkanRenderer.setPlayerController(playerController.uboViewProjection);

    }

    if (key == GLFW_KEY_S){
        forward--;
        printf("S: %f\n", forward);
        playerController.uboViewProjection.view = glm::lookAt(glm::vec3(forward, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -5.0f),
                                                              glm::vec3(0.0f, 1.0f, 0.0f));

        vulkanRenderer.setPlayerController(playerController.uboViewProjection);
    }

}

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


void initWindow(const std::string &wName = "Test Window", const int width = 800, const int height = 600) {
    glfwInit();
    // SET GLFW to NOT work with OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);

    // Init glfw callbacks
    glfwSetErrorCallback(error_callback);
    glfwSetKeyCallback(window, key_callback);

}

int main() {
    initWindow("Vulkan", 1200, 700);

    //Create VulkanRenderer instance
    if (vulkanRenderer.init(window) == EXIT_FAILURE)
        return EXIT_FAILURE;

    // Create player controller
    playerController.uboViewProjection = vulkanRenderer.getPlayerController();

    //Rotation
    double angle = 0.0f;
    double deltaTime = 0.0f;
    double lastTime = 0.0f;


    int helicopter = vulkanRenderer.createMeshModel("../objects/uh60.obj");


    //Loop until close
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;
        if (angle > 360) angle -= 360.0f;


        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(0)), glm::vec3(0.0f, 1.0f, 0.0f));
        testMat = glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(angle), 0.0f, 0.0f));
        vulkanRenderer.updateModel(helicopter, testMat);

        vulkanRenderer.draw();

    }

    std::cout << "Initiate shutdown" << std::endl;

    vulkanRenderer.cleanup();
    //Destroy window and stop GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



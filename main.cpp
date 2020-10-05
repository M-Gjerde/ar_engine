#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "GLFW/glfw3.h"
#include "src/source/GameApplication.h"




GameApplication *app;


static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto* myApp = (AppExtension*)app;
    myApp->keyCallback(window, key, scancode, action, mods);
}

static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
    auto* myApp = (AppExtension*)app;
    app->cursorPosCallback(window, xPos, yPos);
}

int main() {
    app = getApplication();
    glfwSetKeyCallback(app->window, keyCallback);
    glfwSetCursorPosCallback(app->window, cursor_position_callback);
    app->gameLoop();
    return 0;
}



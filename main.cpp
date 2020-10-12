#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "GLFW/glfw3.h"
#include "src/renderer/GameApplication.h"


GameApplication *app;

GameApplication *getApplication() {
    return new AppExtension("AppExtension");
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto* myApp = (AppExtension*)app;
    myApp->keyCallback(window, key, scancode, action, mods);
}

static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
    auto* myApp = (AppExtension*)app;
    app->cursorPosCallback(window, xPos, yPos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* myApp = (AppExtension*)app;
    app->mouseButtonCallback(window, button, action, mods);

}


int main() {
    app = getApplication();
    glfwSetKeyCallback(app->window, keyCallback);
    glfwSetCursorPosCallback(app->window, cursor_position_callback);
    glfwSetMouseButtonCallback(app->window, mouse_button_callback);

    app->gameLoop();
    return 0;
}



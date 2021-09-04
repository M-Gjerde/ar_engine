//
// Created by magnus on 8/28/21.
//

#include "src/GameApplication.h"

GameApplication *application;

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto* myApp = (GameApplication*)application;
    myApp->keyCallback(window, key, scancode, action, mods);
}

static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
    auto* myApp = (GameApplication*)application;
    myApp->cursorPosCallback(window, xPos, yPos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* myApp = (GameApplication*)application;
    myApp->mouseButtonCallback(window, button, action, mods);

}

int main(){

    application = new GameApplication("3D Vision Generator");

    /*
    glfwSetKeyCallback(application->window, keyCallback);
    glfwSetCursorPosCallback(application->window, cursor_position_callback);
    glfwSetMouseButtonCallback(application->window, mouse_button_callback);

     */
    application->gameLoop();

    return 0;
}
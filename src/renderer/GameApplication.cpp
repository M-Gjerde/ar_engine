//
// Created by magnus on 9/21/20.
//

#include <thread>
#include "GameApplication.h"

float frameLimiter = 0.015f;
void AppExtension::update() {
    deltaTime = glfwGetTime() - lastTime;

    uiSettings.FPS = 1.0f / (float) (glfwGetTime() - lastTime);
    uiSettings.frameLimiter = 1 / frameLimiter;
    // Keep update function at 24 frames per second
    if (deltaTime < frameLimiter) {
        auto timeToSleep = (unsigned int) ((frameLimiter - deltaTime) * 1000000);
        usleep(timeToSleep);
    }

    lastTime = glfwGetTime();
}

int cameraIndex = 0;

void AppExtension::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        std::cout << "exiting..." << std::endl;
        vulkanRenderer.stopDisparityStream();
        glfwSetWindowShouldClose(window, true);
    }

    // execute vulkan compute sequence
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        //vulkanRenderer.loadSphereObjects();
        //vulkanRenderer.updateScene();
        vulkanRenderer.startDisparityStream();

    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        vulkanRenderer.stopDisparityStream();

    }

    if (key == GLFW_KEY_UP) {
        if ((1 / frameLimiter) <= 1000)
        frameLimiter -= 0.001f;

    }

    if (key == GLFW_KEY_DOWN) {
        frameLimiter += 0.001f;
    }



    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        vulkanRenderer.testFunction();
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        // Switch to different camera
        cameras[1] = new Camera();
        cameras[1]->cameraPos = glm::vec3(4.0f, -4.5f, 4.0f);
        cameras[1]->cameraFront = glm::vec3(-1.0f, 1.0f, -1.0f);
        cameras[1]->cameraUp = glm::vec3(0.0f, -1.0f, 0.0f);
        cameras[1]->view = glm::lookAt(cameras[1]->cameraPos, cameras[1]->cameraPos + cameras[1]->cameraFront,
                                       cameras[1]->cameraUp);
        cameraIndex++;

        if (cameraIndex == 2)
            cameraIndex = 0;
    }


    if (key == GLFW_KEY_RIGHT)
        cameras[0]->rotateRight();
    if (key == GLFW_KEY_LEFT)
        cameras[0]->rotateLeft();
    if (key == GLFW_KEY_W)
        cameras[0]->forward();
    if (key == GLFW_KEY_S)
        cameras[0]->backward();
    if (key == GLFW_KEY_A)
        cameras[0]->moveLeft();
    if (key == GLFW_KEY_D)
        cameras[0]->moveRight();
    //glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));

    // TODO Make variables inside some class
    glm::mat4 model(1.0f);
    // translate model to camerapos

    model = glm::translate(model, cameras[0]->cameraPos);

    // rotate model according to yaw
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (cameraIndex == 1) {
        model = glm::rotate(model, glm::radians((float) cameras[0]->yaw), glm::vec3(0.0f, -1.0f, 0.0f));
    }

    // Make the model look nice
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
    model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
    vulkanRenderer.objects[0].setModel(model);


    vulkanRenderer.updateCamera(cameras[cameraIndex]->getView(), cameras[cameraIndex]->getProjection());
}

bool hiddenCursor = false;
bool lookAround = false;


void AppExtension::cursorPosCallback(GLFWwindow *window, double _xPos, double _yPos) {
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2((float) _xPos, (float) _yPos);


    if (lookAround) {
        cameras[0]->lookAround(_xPos, _yPos);

        // If we are in 3rd person mode then disable up and down movement
        if (cameraIndex == 1) {
            cameras[0]->cameraFront.y = 0.0f;
            cameras[0]->cameraFront = glm::normalize(cameras[0]->cameraFront);;
        }

        // Only update 1st person camera for cursor callback
        if (cameraIndex == 0) {
            vulkanRenderer.updateCamera(cameras[0]->getView(), cameras[0]->getProjection());
        }
    }
}


void AppExtension::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {

    ImGuiIO &io = ImGui::GetIO();


    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        io.MouseDown[0] = true;

        const bool hover = ImGui::IsAnyItemHovered();

        if (hiddenCursor) {
            hiddenCursor = false;
            lookAround = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); //Normal mouse mode input
        } else if (!hover){
            hiddenCursor = true;
            lookAround = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // FPS mode mouse input
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        io.MouseDown[0] = false;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        io.MouseDown[1] = true;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        io.MouseDown[1] = false;
    }



/*

        printf("____________________________________________\n "
               "Camera data: yaw = %f\n", cameras[cameraIndex]->yaw);

        printf("cameraFront (x,y,z): %f, %f, %f\n", cameras[cameraIndex]->cameraFront.x,
cameras[cameraIndex]->cameraFront.y, cameras[cameraIndex]->cameraFront.z);

*/

}



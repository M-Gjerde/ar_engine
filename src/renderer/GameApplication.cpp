//
// Created by magnus on 9/21/20.
//

#include <zconf.h>
#include <thread>
#include "GameApplication.h"

bool rotate = false;
float angle = 0;
float rColor = 0;
glm::mat4 rotateMat;

glm::vec3 lightPos = glm::vec3(5.0f, 0.0f, 3.0f);
glm::mat4 lightTrans(1.0f);

void AppExtension::update() {

    double now = glfwGetTime();
    deltaTime = now - lastTime;
    lastTime = now;


    if (rotate) {
        angle += 50.0f * static_cast<float>(deltaTime);
        if (angle > 360) angle -= 360.0f;
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
        rotateMat = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        rotateMat = glm::rotate(rotateMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        vulkanRenderer.updateModel(rotateMat, 2, false);

    }


    /*
    // Keep update function at 24 frames per second
    if (deltaTime < 0.03) {
        double timeToSleep = (0.03 - deltaTime) * 1000;
        usleep(timeToSleep);
    }
*/

    //vulkanRenderer.updateSpecularLightCamera(camera.cameraPos);
}


float forward = 5, right = 0, rotation = 0;
float model_angle = 0;
int modelIndex = 1;

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

    float rotAngle = 1;
    std::vector<SceneObject> objects = vulkanRenderer.getSceneObjects();
    glm::mat4 model = objects[modelIndex].getModel();
    glm::vec3 scale = objects[modelIndex].getScaleVector();
    float increment = (float) 0.25 / scale[0];
    switch (key) {
        case GLFW_KEY_KP_8:
            model = glm::translate(model, glm::vec3(0, -increment, 0));
            break;
        case GLFW_KEY_KP_2:
            model = glm::translate(model, glm::vec3(0, increment, 0));
            break;
        case GLFW_KEY_KP_4:
            model = glm::translate(model, glm::vec3(increment, 0, 0));
            break;
        case GLFW_KEY_KP_6:
            model = glm::translate(model, glm::vec3(-increment, 0, 0));
            break;
        case GLFW_KEY_KP_7:
            model = glm::rotate(model, glm::radians(-rotAngle), glm::vec3(0, 0, 1));
            break;
        case GLFW_KEY_KP_9:
            model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0, 0, 1));
            break;
        default:
            break;
    }

    if (modelIndex == 1) {
        vulkanRenderer.updateModel(model, modelIndex, true);
    }

    vulkanRenderer.updateModel(model, modelIndex, false);

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        // Load scene objects according to settings file
        vulkanRenderer.resetScene();
        auto settingsMap = loadSettings.getSceneObjects();
        //vulkanRenderer.setupSceneFromFile(settingsMap);
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        vulkanRenderer.stopDisparityStream();

    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        modelIndex++;
        printf("modelIndex: %d\n", modelIndex);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        modelIndex--;
        printf("modelIndex: %d\n", modelIndex);

    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS) {


    }


    if (key == GLFW_KEY_RIGHT)
        camera.rotateRight();
    if (key == GLFW_KEY_LEFT)
        camera.rotateLeft();
    if (key == GLFW_KEY_W)
        camera.forward();
    if (key == GLFW_KEY_S)
        camera.backward();
    if (key == GLFW_KEY_A)
        camera.moveLeft();
    if (key == GLFW_KEY_D)
        camera.moveRight();


    vulkanRenderer.updateCamera(camera.getView(), camera.getProjection());

}

double xPos, yPos;

void AppExtension::cursorPosCallback(GLFWwindow *window, double _xPos, double _yPos) {
    xPos = _xPos;
    yPos = _yPos;

}

void AppExtension::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        printf("Mouse click at pos: %f, %f\n", xPos, yPos);

        printf("____________________________________________\n "
               "Camera data: yaw = %f\n", camera.yaw);

        printf("cameraFront (x,y,z): %f, %f, %f\n", camera.cameraFront.x, camera.cameraFront.y, camera.cameraFront.z);
    }

}


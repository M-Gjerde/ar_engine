//
// Created by magnus on 8/28/21.
//

#include "src/Renderer.h"

Renderer *application;


int main() {

    application = new Renderer("3D Vision Generator");
    application->prepareEngine();
    application->run();
    return 0;
}
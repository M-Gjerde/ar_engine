//
// Created by magnus on 8/28/21.
//

#include "src/GameApplication.h"

GameApplication *application;


int main(){

    application = new GameApplication("3D Vision Generator");

    application->gameLoop();

    return 0;
}
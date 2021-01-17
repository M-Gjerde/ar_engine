//
// Created by magnus on 1/17/21.
//

#include <unistd.h>
#include <csignal>
#include "ThreadSpawner.h"

void ThreadSpawner::startStream() {

    if (isStreamRunning()) {
        printf("Streamer already running\n");
        return;
    }

    pidStreamer = fork();

    // Parent process
     if (pidStreamer > 0) {
        status = true;
        return;
    }

    // Child process
    // TODO Launch grabframe from here
    else if (pidStreamer == 0) {
        // child process
         printf("Launching streamer\n");
        int i = 0;
        for (; i < 5; ++i) {
        }
    }



    while (true);

}

void ThreadSpawner::stopStream() {

    if (!isStreamRunning()) {
        printf("Streamer is not running\n");
    } else {
        kill(pidStreamer, SIGKILL);
        status = false;
        pidStreamer = -1;
    }
}

ThreadSpawner::ThreadSpawner() {

}

bool ThreadSpawner::isStreamRunning() {
    return status;
}

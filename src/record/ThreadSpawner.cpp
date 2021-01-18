//
// Created by magnus on 1/17/21.
//


#include <stdexcept>
#include <cstring>
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
    else if (pidStreamer == 0)
        childProcess();

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
    // Create shared memory segment
    memID = shmget(IPC_PRIVATE, sizeof(shMem), 0666 | IPC_CREAT);
    if (memID == -1) {
        throw std::runtime_error("Failed to create shared memory segment\n");
    }

    printf("Attaching to shared memory ");
    auto *memP = (shMem *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }

    printf("Parent pointer addr: %p\n", &memP);


}

bool ThreadSpawner::isStreamRunning() {
    return status;
}

void ThreadSpawner::childProcess() {

    printf("Attaching to shared memory ");
    auto *memP = (shMem *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }
    printf("pointer addr: %p\n", &memP);
    memset(memP->buffer, 99, 1024);

    printf("Detaching shared memory\n");
    if (shmdt(memP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }


    while (true);

}

void ThreadSpawner::initV4l2() {

}

void ThreadSpawner::readMemory() {

    auto *memP = (shMem *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }
    printf("Pointer addr: %p\n", &memP);

    for (int i = 0; i < 1; ++i) {
        printf("Data at pointer: %d\n", memP->buffer[i]);
    }

    if (shmdt(memP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }

}

//
// Created by magnus on 1/17/21.
//

#ifndef AR_ENGINE_THREADSPAWNER_H
#define AR_ENGINE_THREADSPAWNER_H


#include <string>
#include <unistd.h>
#include <csignal>
#include <sys/ipc.h>
#include <sys/shm.h>

class ThreadSpawner {

public:
    ThreadSpawner();

    void startStream();
    void stopStream();


    void readMemory();

private:
    std::string cmd;
    pid_t pidStreamer = -1;
    bool status = false;
    int memID;
    uint32_t memorySize = 1280 * 720;
    struct shMem {
        unsigned char buffer[1024];
    };

    void childProcess();

    void initV4l2();
    void captureFrame();
    bool isStreamRunning();

};


#endif //AR_ENGINE_THREADSPAWNER_H

//
// Created by magnus on 1/17/21.
//

#ifndef AR_ENGINE_THREADSPAWNER_H
#define AR_ENGINE_THREADSPAWNER_H


#include <string>

class ThreadSpawner {

public:
    ThreadSpawner();

    void startStream();
    void stopStream();
    bool isStreamRunning();


private:
    std::string cmd;
    pid_t pidStreamer = -1;
    bool status = false;

};


#endif //AR_ENGINE_THREADSPAWNER_H

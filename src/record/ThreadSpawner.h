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
#include <libv4l2rds.h>
#include <stdexcept>
#include <cstring>
#include <libv4l2.h>
#include <mqueue.h>
#include <sys/mman.h>
#include "../include/structs.h"

class ThreadSpawner {

public:
    ThreadSpawner();

    void startChildProcess();

    void stopChildProcess();
    bool isStreamRunning();


    ArSharedMemory * readMemory();

private:
    pid_t pidStreamer = -1;
    bool status = false;
    int memID;


    struct Buffer {
        void *start;
        size_t length;
    };

    void childProcess();





    void xioctl(int fh, int request, void *arg);

    int run();
    static int openDevice(char *dev_name);
    void setMode(uint fd, int mode);
    void imageProperties(int fd, int width, int height);
    void setupBuffers(int fd, v4l2_buffer *buf, Buffer *pBuffer);

    void startVideoStream(int fd);
    void stopVideoStream(int fd);

    [[nodiscard]] ArSharedMemory *attachMemory() const;

    static void detachMemory(ArSharedMemory *memP);
};


#endif //AR_ENGINE_THREADSPAWNER_H

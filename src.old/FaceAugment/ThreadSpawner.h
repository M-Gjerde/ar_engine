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
#include <sys/ioctl.h>
#include <array>
#include <iostream>

#include <opencv2/opencv.hpp>
#include "../include/structs.h"
#include "RealsenseStreamer.h"


class ThreadSpawner {

public:
    ThreadSpawner();

    void startChildProcess();

    void stopChildProcess();
    bool isStreamRunning() const;


    ArSharedMemory * getVideoMemoryPointer() const;
    Status * getStatusMemoryPointer() const;

    void waitForExistence() const;

private:
    pid_t pidStreamer = -1;
    int shVideoMem;
    int shStatusMem;

    struct Buffer {
        void *start;
        size_t length;
    };


    void childProcess();

    void xioctl(int fh, int request, void *arg);

    int setupAndRunVideoStream();
    static int openDevice(char *dev_name);
    void setMode(uint fd, int mode);
    void imageProperties(int fd, int width, int height);
    void setupBuffers(int fd, v4l2_buffer *buf, Buffer *pBuffer);

    void startVideoStream(int fd);
    void stopVideoStream(int fd);

    [[nodiscard]] ArSharedMemory *attachMemory() const;

    static void detachMemory(ArSharedMemory *memP);

    void realsenseVideoStream();

    void setChildProcessStatus(bool status);
};


#endif //AR_ENGINE_THREADSPAWNER_H

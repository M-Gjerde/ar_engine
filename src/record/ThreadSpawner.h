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

class ThreadSpawner {

public:
    ThreadSpawner();

    void startChildProcess();

    void stopChildProcess();


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

    struct Buffer {
        void *start;
        size_t length;
    };

    struct ArV4l2 {
        v4l2_format fmt;
        v4l2_buffer v4l2Buffer[2];
        v4l2_requestbuffers reqBuffers[2];
        v4l2_buf_type bufferType;

        fd_set fds[2];
        int fd[2];

        std::string deviceNames[2] = {"/dev/video0", "/dev/video1"};

        Buffer buffers[2];
    } arV4L2;

    void childProcess();

    void initV4l2();

    void startVideoStream();

    void stopVideoStream();

    void captureFrame();

    bool isStreamRunning();

    void xioctl(int fh, int request, void *arg);

    void setSensorMode();

    void setImageProperties();

    void initBuffers();

    void grabFrame();
};


#endif //AR_ENGINE_THREADSPAWNER_H

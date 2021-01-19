//
// Created by magnus on 1/17/21.
//


#include <stdexcept>
#include <cstring>
#include <libv4l2rds.h>
#include <libv4l2.h>
#include <mqueue.h>
#include <sys/mman.h>
#include "opencv2/opencv.hpp"

#include "ThreadSpawner.h"

void ThreadSpawner::xioctl(int fh, int request, void *arg) {
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }


}

void ThreadSpawner::startChildProcess() {

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

void ThreadSpawner::stopChildProcess() {

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


    initV4l2();
    setSensorMode();
    setImageProperties();
    initBuffers();

    startVideoStream();
    grabFrame();
    stopVideoStream();


    while (true);

}

void ThreadSpawner::initV4l2() {

    for (int i = 0; i < 2; ++i) {
        arV4L2.fd[i] = v4l2_open(arV4L2.deviceNames[i].c_str(), O_RDWR | O_NONBLOCK, 0);

        printf("name: %s, fd %d\n", arV4L2.deviceNames[i].c_str(), arV4L2.fd[i]);
        if (arV4L2.fd[i] < 0) {
            perror("Cannot open device");
            exit(EXIT_FAILURE);
        }
    }


}

void ThreadSpawner::setSensorMode() {

}

void ThreadSpawner::setImageProperties() {

    int width = 1280, height = 720;
    memset(&arV4L2.fmt, 0, sizeof(arV4L2.fmt));
    arV4L2.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    arV4L2.fmt.fmt.pix.width = width;
    arV4L2.fmt.fmt.pix.height = height;
    arV4L2.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB10;
    arV4L2.fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    for (int i : arV4L2.fd) {
        xioctl(i, VIDIOC_S_FMT, &arV4L2.fmt);
        if (arV4L2.fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SRGGB10) {
            printf("Libv4l didn't accept RGGB10 format. Can't proceed.\n");
            exit(EXIT_FAILURE);
        }
        if ((arV4L2.fmt.fmt.pix.width != width) || (arV4L2.fmt.fmt.pix.height != height))
            throw std::runtime_error("Failed to set image properties");
    }


}

void ThreadSpawner::initBuffers() {
    for (int i = 0; i < 2; ++i) {
        // Set up request buffers
        memset(&arV4L2.reqBuffers[i], 0x00, sizeof(arV4L2.reqBuffers[i]));
        arV4L2.reqBuffers[i].count = 1;
        arV4L2.reqBuffers[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        arV4L2.reqBuffers[i].memory = V4L2_MEMORY_MMAP;
        xioctl(arV4L2.fd[i], VIDIOC_REQBUFS, &arV4L2.reqBuffers[i]);

        // TODO CALLOC HERE OF MY BUFFERS
        //arV4L2.buffers[i] = static_cast<Buffer *>(calloc(arV4L2.reqBuffers[i].count, sizeof(*Buffers)));

        // Further setup of req buffers
        memset(&arV4L2.v4l2Buffer[i], 0x00, sizeof(arV4L2.v4l2Buffer[i]));
        arV4L2.v4l2Buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        arV4L2.v4l2Buffer[i].memory = V4L2_MEMORY_MMAP;
        arV4L2.v4l2Buffer[i].index = i;
        xioctl(arV4L2.fd[i], VIDIOC_QUERYBUF, &arV4L2.v4l2Buffer[i]);

        // Map buffers from v4l2 buffers
        arV4L2.buffers[i].length = arV4L2.v4l2Buffer[i].length;
        arV4L2.buffers[i].start = v4l2_mmap(NULL, arV4L2.v4l2Buffer[i].length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                            arV4L2.fd[i], arV4L2.v4l2Buffer[i].m.offset);

        if (MAP_FAILED == arV4L2.buffers[i].start) {
            perror("Failed to v4l2 map buffer");
            exit(EXIT_FAILURE);
        }

        // Clear request buffers - I think
        memset(&arV4L2.reqBuffers[i], 0x00, sizeof(arV4L2.reqBuffers[i]));
        arV4L2.v4l2Buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        arV4L2.v4l2Buffer[i].memory = V4L2_MEMORY_MMAP;
        arV4L2.v4l2Buffer[i].index = i;
        xioctl(arV4L2.fd[i], VIDIOC_QBUF, &arV4L2.v4l2Buffer[i]);
    }

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

void ThreadSpawner::startVideoStream() {
    for (int i : arV4L2.fd) {
        arV4L2.bufferType = V4L2_BUF_TYPE_VBI_CAPTURE;
        xioctl(i, VIDIOC_STREAMON, &arV4L2.bufferType);
    }

}

void ThreadSpawner::stopVideoStream() {
    for (int i : arV4L2.fd) {
        arV4L2.bufferType = V4L2_BUF_TYPE_VBI_CAPTURE;
        xioctl(i, VIDIOC_STREAMOFF, &arV4L2.bufferType);
    }
}

void ThreadSpawner::grabFrame() {

    int r = -1;
    timeval tv{};
    clock_t Start = clock();

    cv::namedWindow("window", cv::WINDOW_NORMAL);
    cv::namedWindow("window2", cv::WINDOW_NORMAL);

    // Select devices
    for (int i = 0; i < 2; ++i) {
        FD_ZERO(&arV4L2.fds[i]);
        FD_SET(arV4L2.fd[i], &arV4L2.fds[i]);
        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        r = select(arV4L2.fd[i] + 1, &arV4L2.fds[i], NULL, NULL, &tv);

        if (r == -1 && (errno = EINTR)) {
            perror("select");
            exit(EXIT_FAILURE);
        }
    }

    // Select buffers
    for (int i = 0; i < 2; ++i) {

        // Further setup of req buffers
        memset(&arV4L2.v4l2Buffer[i], 0x00, sizeof(arV4L2.v4l2Buffer[i]));
        arV4L2.v4l2Buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        arV4L2.v4l2Buffer[i].memory = V4L2_MEMORY_MMAP;
        xioctl(arV4L2.fd[i], VIDIOC_DQBUF, &arV4L2.v4l2Buffer[i]);

    }

    // Copy data
    void *pixelData[2];
    for (int i = 0; i < 2; ++i) {
        pixelData[i] = arV4L2.buffers[arV4L2.v4l2Buffer[i].index].start;
    }
    auto endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
    printf("copy time taken: %.7fs \n", endTime);

    // Display images
    uint16_t *d[2]; // pixeldata
    size_t imageSize = arV4L2.buffers[arV4L2.v4l2Buffer[0].index].length / 2;


    std::vector<uchar> newPixels;
    std::vector<uchar> newPixels2;
    newPixels.resize(imageSize);
    newPixels2.resize(imageSize);
    int min = 1000, max = 0;
    int prev = 0;
    int pixMax = 255, pixMin = 50;
    for (int j = 0; j < imageSize; ++j) {
        uchar newVal = (255 - 0) / (pixMax - pixMin) * (*d[0] - pixMax) + 255;
        uchar newVal2 = (255 - 0) / (pixMax - pixMin) * (*d[1] - pixMax) + 255;
        newPixels.at(j) = newVal;
        newPixels2.at(j) = newVal2;
        //newPixels.push_back(newVal);
        d[0]++;
        d[1]++;
    }


    printf("val: %d, size: %zu\n", newPixels[0], newPixels.size());
    endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
    printf("Loop time taken: %.7fs\n", endTime);

    cv::Mat outputImage(1280, 720, CV_8U);
    cv::Mat outputImage2(1280, 720, CV_8U);

    outputImage.data = newPixels.data();
    outputImage2.data = newPixels2.data();


    cv::imshow("window2", outputImage2);
    cv::imshow("window", outputImage);
    cv::waitKey(5);

    endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
    printf("Total time taken: %.7fs\n\n", endTime);

    for (int i = 0; i < 2; ++i) {
        xioctl(arV4L2.fd[i], VIDIOC_QBUF, &arV4L2.v4l2Buffer[i]);

    }
}


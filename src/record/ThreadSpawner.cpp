//
// Created by magnus on 1/17/21.
//




#include <sys/ioctl.h>
#include <array>
#include "ThreadSpawner.h"

#define WIDTH 1280
#define HEIGHT 720
#define CLEAR(x) memset(&(x), 0, sizeof(x))

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
    memID = shmget(IPC_PRIVATE, sizeof(ArSharedMemory), 0666 | IPC_CREAT);
    if (memID == -1) {
        throw std::runtime_error("Failed to create shared memory segment\n");
    }

    auto *memP = (ArSharedMemory *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }

}

bool ThreadSpawner::isStreamRunning() {
    return status;
}

void ThreadSpawner::childProcess() {
    run();

    while (true);

}


ArSharedMemory * ThreadSpawner::readMemory() {

    auto *memP = (ArSharedMemory *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }

    // TODO DETACH AT SOME OTHER POINT
   /* if (shmdt(memP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }
*/
    return memP;
}

void ThreadSpawner::startVideoStream(int fd) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);
}


void ThreadSpawner::stopVideoStream(int fd) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

}


struct Buffer {
    void *start;
    size_t length;
};

void ThreadSpawner::setMode(uint fd, int mode) {
    // -- SET SENSOR MODE IN CODE --
    v4l2_ext_controls ctrls{};
    v4l2_ext_control ctrl{};
    memset(&ctrls, 0, sizeof(ctrls));
    memset(&ctrl, 0, sizeof(ctrl));
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(0x009a2008);  // TODO make dynamic
    ctrls.count = 1;
    ctrls.controls = &ctrl;
    ctrl.id = 0x009a2008; // TODO make dynamic
    ctrl.value64 = mode;

    xioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls);
}

int ThreadSpawner::openDevice(char *dev_name) {
    return v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);

}


void ThreadSpawner::imageProperties(int fd, int width, int height) {
    // -- SET IMAGE PROPERTIES
    v4l2_format fmt{};

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB10;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    xioctl(fd, VIDIOC_S_FMT, &fmt);

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SRGGB10) {
        printf("Libv4l didn't accept RGGB10 format. Can't proceed.\n");
        exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != width) || (fmt.fmt.pix.height != height))
        printf("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

}

void ThreadSpawner::setupBuffers(int fd, v4l2_buffer *buf, Buffer *pBuffer) {
    struct v4l2_requestbuffers req{};
    std::array<Buffer, 3> tempBuff{};


    // -- TODO EXPLAIN BUFFER STUFF
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    xioctl(fd, VIDIOC_REQBUFS, &req);
    int n_buffers;


    //buffer = static_cast<Buffer *>(calloc(req.count, sizeof(*buffer)));
    //pBuffer = static_cast<Buffer *>(calloc(req.count, sizeof(*pBuffer)));

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(*buf);

        buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf->memory = V4L2_MEMORY_MMAP;
        buf->index = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, buf);

        tempBuff[n_buffers].length = buf->length;
        tempBuff[n_buffers].start = v4l2_mmap(nullptr, buf->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                                            buf->m.offset);

        if (MAP_FAILED == pBuffer[n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        *pBuffer = tempBuff[n_buffers];

        pBuffer++;

    }

    for (int i = 0; i < n_buffers; ++i) {
        CLEAR(*buf);
        buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf->memory = V4L2_MEMORY_MMAP;
        buf->index = i;
        xioctl(fd, VIDIOC_QBUF, buf);
    }

}

ArSharedMemory * ThreadSpawner::attachMemory() const{
    auto *memP = (ArSharedMemory *) shmat(memID, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }

    return memP;
}

void ThreadSpawner::detachMemory(ArSharedMemory * memP){
    if (shmdt(memP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }
}

int ThreadSpawner::run() {
    //v4l2_buffer buf{}, buf2{};
    v4l2_buffer v4l2buffers[2];
    enum v4l2_buf_type type;
    fd_set fds, fds2;
    timeval tv{};
    int r;
    //Buffer buffer1[3];
    //Buffer buffer2[3];
    std::array<Buffer, 3> buffer1{};
    std::array<Buffer, 3> buffer2{};

    std::string devName = "/dev/video0";
    std::string devName2 = "/dev/video1";
    int fd[2] = {openDevice(const_cast<char *>(devName.c_str())), openDevice(const_cast<char *>(devName2.c_str()))};

    // For each file descriptor
    for (int j : fd) {
        setMode(j, 3);
        imageProperties(j, WIDTH, HEIGHT);
    }

    printf("Setting up buffers\n");
    setupBuffers(fd[0], &v4l2buffers[0], buffer1.data());
    setupBuffers(fd[1], &v4l2buffers[1], buffer2.data());
    auto k = buffer1[0];
    printf("end setup\n");
    // END
    // imgOne videoStream
    for (int j : fd) {
        startVideoStream(j);
    }
    // initialization
    //cv::namedWindow("window", cv::WINDOW_NORMAL);
    //cv::namedWindow("window2", cv::WINDOW_NORMAL);
    while (true) {
        clock_t Start = clock();
        do {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);

            FD_ZERO(&fds2);
            FD_SET(fd[1], &fds2);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(fd[0] + 1, &fds, NULL, NULL, &tv);
            r = select(fd[1] + 1, &fds2, NULL, NULL, &tv);

        } while ((r == -1 && (errno = EINTR)));
        if (r == -1) {
            perror("select");
            return errno;
        }

        CLEAR(v4l2buffers[0]);
        v4l2buffers[0].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2buffers[0].memory = V4L2_MEMORY_MMAP;
        xioctl(fd[0], VIDIOC_DQBUF, &v4l2buffers[0]);

        CLEAR(v4l2buffers[1]);
        v4l2buffers[1].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2buffers[1].memory = V4L2_MEMORY_MMAP;
        xioctl(fd[1], VIDIOC_DQBUF, &v4l2buffers[1]);


        // Attach
        ArSharedMemory* memP = attachMemory();
        // Copy data
        memcpy(memP->imgOne, buffer1[v4l2buffers[0].index].start, buffer1[v4l2buffers[0].index].length);
        memcpy(memP->imgTwo, buffer2[v4l2buffers[0].index].start, buffer2[v4l2buffers[0].index].length);
        memP->imgLen1 = buffer1[v4l2buffers[0].index].length;
        memP->imgLen2 = buffer2[v4l2buffers[1].index].length;
        // Detach
        detachMemory(memP);


        /*
        uint16_t *d = (uint16_t *) pixelData;
        uint16_t *d2 = (uint16_t *) pixelData2;
        std::vector<uchar> newPixels;
        std::vector<uchar> newPixels2;
        newPixels.resize(imageSize);
        newPixels2.resize(imageSize);


        int pixMax = 255, pixMin = 50;
        for (int j = 0; j < imageSize; ++j) {
            uchar newVal = (255 - 0) / (pixMax - pixMin) * (*d - pixMax) + 255;
            uchar newVal2 = (255 - 0) / (pixMax - pixMin) * (*d2 - pixMax) + 255;
            newPixels.at(j) = newVal;
            newPixels2.at(j) = newVal2;

            d++;
            d2++;
        }



        cv::Mat outputImage(HEIGHT, WIDTH, CV_8U);
        cv::Mat outputImage2(HEIGHT, WIDTH, CV_8U);

        outputImage.data = newPixels.data();
        outputImage2.data = newPixels2.data();

        //cv::imshow("window2", outputImage2);
        //cv::imshow("window", outputImage);

        if (cv::waitKey(30) == 27) break;
*/


        xioctl(fd[0], VIDIOC_QBUF, &v4l2buffers[0]);
        xioctl(fd[1], VIDIOC_QBUF, &v4l2buffers[1]);

        //endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
        //printf("Total time taken: %.7fs\n\n", endTime);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd[0], VIDIOC_STREAMOFF, &type);
    xioctl(fd[1], VIDIOC_STREAMOFF, &type);

    /* for (i = 0; i < n_buffers; ++i)
         v4l2_munmap(buffers[i].imgOne, buffers[i].imgLen1);
     v4l2_close(fd[0]);

     for (i = 0; i < n_buffers2; ++i)
         v4l2_munmap(buffers2[i].imgOne, buffers2[i].imgLen1);
     v4l2_close(fd[1]);

 */
    return 0;
}


//
// Created by magnus on 1/17/21.
//


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
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
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
        return;
    }

        // Child process
    else if (pidStreamer == 0) {
        childProcess();

    }

}

void ThreadSpawner::stopChildProcess() {
    printf("Is streamrunning: %d\n", isStreamRunning());
    if (!isStreamRunning()) {
        printf("Streamer is not running\n");
    } else {
        kill(pidStreamer, SIGKILL);
        pidStreamer = -1;
    }
}

ThreadSpawner::ThreadSpawner() {
    // Create shared memory segment
    shVideoMem = shmget(IPC_PRIVATE, sizeof(ArSharedMemory), 0666 | IPC_CREAT);
    if (shVideoMem == -1) {
        throw std::runtime_error("Failed to create shared memory segment\n");
    }

    auto *vidMemP = (ArSharedMemory *) shmat(shVideoMem, nullptr, 0);
    if (vidMemP == (void *) -1) {
        throw std::runtime_error("Failed to attach to video memory segment");
    }

    shStatusMem = shmget(IPC_PRIVATE, sizeof(Status), 0666 | IPC_CREAT);
    if (shStatusMem == -1) throw std::runtime_error("Failed to create isRunning memory segment");

    auto *statusMemP = (Status *) shmat(shStatusMem, nullptr, 0);
    if (statusMemP == (void *) -1) {
        throw std::runtime_error("Failed to attach to isRunning memory segment");
    }

}

bool ThreadSpawner::isStreamRunning() const {

    auto *statusMemP = (Status *) shmat(shStatusMem, nullptr, 0);
    if (statusMemP == (void *) -1) {
        throw std::runtime_error("Failed to attach to isRunning memory segment");
    }
    bool isRunning = statusMemP->isRunning;

    if (shmdt(statusMemP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }

    return isRunning;
}

void ThreadSpawner::childProcess() {
    // setupAndRunVideoStream(); For V4L2 Framework

    // For Realsense camera stream
    realsenseVideoStream();

}

void ThreadSpawner::realsenseVideoStream() {
    try {

        int SCR_WIDTH = 640;
        int SCR_HEIGHT = 480;
        // -- REALSENSE ---
        // Create initial config for streaming
        rs2::config cfg;


        //Add desired streams to configuration
        //cfg.enable_stream(RS2_STREAM_COLOR, 1920, 1080, RS2_FORMAT_BGR8, 30);
        cfg.enable_stream(RS2_STREAM_DEPTH, SCR_WIDTH, SCR_HEIGHT, RS2_FORMAT_Z16, 30);
        cfg.enable_stream(RS2_STREAM_INFRARED, 1, SCR_WIDTH, SCR_HEIGHT, RS2_FORMAT_Y8, 30);
        cfg.enable_stream(RS2_STREAM_INFRARED, 2, SCR_WIDTH, SCR_HEIGHT, RS2_FORMAT_Y8, 30);

        RealsenseStreamer realsenseStreamer(cfg);
        realsenseStreamer.startStream();
        realsenseStreamer.streaming();

        // --- Update child process STATUS
        setChildProcessStatus(true);

        // Disable emitter
        realsenseStreamer.setEmitter(0);

        // uncomment this call to draw in wireframe polygons.
        //cv::namedWindow("depth", cv::WINDOW_AUTOSIZE);

        while (realsenseStreamer.streaming()) {

            // Retrieve depth images
            cv::Mat ir_right = realsenseStreamer.getImage(realsenseStreamer.IR_IMAGE_RIGHT);
            cv::Mat ir_left = realsenseStreamer.getImage(realsenseStreamer.IR_IMAGE_LEFT);

            // Copy data to shared memory
            //cv::imshow("ir_right", ir_right);
            //cv::imshow("ir_left", ir_left);

            // Attach
            ArSharedMemory *memP = attachMemory();
            // Copy data
            memcpy(memP->imgOne, ir_left.data, ir_left.cols * ir_left.rows);
            memcpy(memP->imgTwo, ir_right.data, ir_right.cols * ir_right.rows);
            memP->imgLen1 = ir_left.cols * ir_left.rows;
            memP->imgLen2 = ir_right.cols * ir_right.rows;
            // Detach
            detachMemory(memP);

            // Display images
            if (cv::waitKey(30) == 27)
            {
                setChildProcessStatus(false);
                realsenseStreamer.stopStream();
                break;
            }

        }
    }
    catch (std::runtime_error &error) {
        printf("We got a realsense error: %s\n", error.what());
    }

    catch (const rs2::error& e)
    {
        std::cerr << "Some other error occurred!" << std::endl;
    }

    cv::destroyAllWindows();


}

Status *ThreadSpawner::getStatusMemoryPointer() const {

    auto *memP = (Status *) shmat(shStatusMem, nullptr, 0);
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

// TODO Write this with templates to avoid repeating the same code for getStatusMemoryPointer
ArSharedMemory *ThreadSpawner::getVideoMemoryPointer() const {

    auto *memP = (ArSharedMemory *) shmat(shVideoMem, nullptr, 0);
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


ArSharedMemory *ThreadSpawner::attachMemory() const {
    auto *memP = (ArSharedMemory *) shmat(shVideoMem, nullptr, 0);
    if (memP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }
    return memP;
}

void ThreadSpawner::detachMemory(ArSharedMemory *memP) {
    if (shmdt(memP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }
}


void ThreadSpawner::startVideoStream(int fd) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);
}


void ThreadSpawner::stopVideoStream(int fd) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

}


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


int ThreadSpawner::setupAndRunVideoStream() {
    v4l2_buffer v4l2buffers[2];
    fd_set fds, fds2;
    timeval tv{};
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

    setupBuffers(fd[0], &v4l2buffers[0], buffer1.data());
    setupBuffers(fd[1], &v4l2buffers[1], buffer2.data());

    for (int j : fd) {
        startVideoStream(j);
    }
    int r = -1;

    setChildProcessStatus(true);


    while (true) {
        do {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);

            FD_ZERO(&fds2);
            FD_SET(fd[1], &fds2);

            /* Timeout. */
            tv.tv_sec = 7;
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
        ArSharedMemory *memP = attachMemory();
        // Copy data
        memcpy(memP->imgOne, buffer1[v4l2buffers[0].index].start, buffer1[v4l2buffers[0].index].length);
        memcpy(memP->imgTwo, buffer2[v4l2buffers[0].index].start, buffer2[v4l2buffers[0].index].length);
        memP->imgLen1 = buffer1[v4l2buffers[0].index].length;
        memP->imgLen2 = buffer2[v4l2buffers[1].index].length;
        // Detach
        detachMemory(memP);


        xioctl(fd[0], VIDIOC_QBUF, &v4l2buffers[0]);
        xioctl(fd[1], VIDIOC_QBUF, &v4l2buffers[1]);

    }

    for (int i : fd) {
        stopVideoStream(i);

    }

    // TODO V4L2 MunMap
    /*
    for (int i = 0; i < buffer1.size(); ++i)
         v4l2_munmap(buffer1[i].imgOne, buffers[i].imgLen1);
     v4l2_close(fd[0]);

     for (int i = 0; i < n_buffers2; ++i)
         v4l2_munmap(buffers2[i].imgOne, buffers2[i].imgLen1);
     v4l2_close(fd[1]);
*/

    return 0;
}


void ThreadSpawner::setChildProcessStatus(bool status){
    // --- Update child process STATUS
    auto *statusMemP = (Status *) shmat(shStatusMem, nullptr, 0);
    if (statusMemP == (void *) -1) {
        throw std::runtime_error("Failed to attach to shared memory segment");
    }
    statusMemP->isRunning = status;
    if (shmdt(statusMemP) == -1) {
        throw std::runtime_error("Failed to detach to shared memory");
    }
    // --- END update child process STATUS
}


void ThreadSpawner::waitForExistence() const {
    clock_t Start = clock();
    double timeOut_ms = 2000;

    auto memP = getStatusMemoryPointer();

    while (!memP->isRunning) {
        auto endTime = (double) (clock() - Start) / CLOCKS_PER_SEC * 1000;
        if (endTime > timeOut_ms) throw std::runtime_error("Failed to start camera process within reasonable time\n");

    }

}


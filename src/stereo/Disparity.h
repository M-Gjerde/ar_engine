//
// Created by magnus on 10/8/20.
//

#ifndef AR_ENGINE_DISPARITY_H
#define AR_ENGINE_DISPARITY_H


#include <thread>
#include <fstream>
#include <iostream>
#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <CL/cl2.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>


class Disparity {
public:
    Disparity() = default;
    int init();

    void cleanUp();
    ~Disparity() = default;
    int input = 0;
    void stopProgram();

    void getDisparityFromImage(unsigned char *data);
    static void getDisparityFromVideo();

    size_t imageSize;
    int imageWidth, imageHeight;

private:

    // Init variables
    bool run_status = false;
    std::thread check_for_actions_thread{};

    // Runtime things

    // Functions
    void checkForActions();

    static std::string loadProgram(std::string input);
    static cl::Device getGPUDevice();
};



#endif //AR_ENGINE_DISPARITY_H

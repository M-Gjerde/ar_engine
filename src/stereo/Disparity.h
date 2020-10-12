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

private:

    // Init variables
    bool run_status = false;
    std::thread check_for_actions_thread{};

    // Runtime things

    // Functions
    void checkForActions();
    void getDisparityFromImage();
    static void getDisparityFromVideo();

    static std::string loadProgram(std::string input);
    static cl::Device getGPUDevice();
};



#endif //AR_ENGINE_DISPARITY_H

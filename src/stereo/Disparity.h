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

    void getDisparityFromImage(unsigned char **data);
    void getDisparityFromVideo();

    size_t imageSize;
    int imageWidth, imageHeight;
    unsigned char* pixelData;
    bool pixelDataReady = false;
    bool resourceBusy = false;


private:

    // Init variables
    bool run_status = false;
    std::thread threadCheckInput{};
    std::thread threadProcessDisparity{};
    bool disparityInProgress = false;

    // Runtime things

    // Functions
    void checkForActions();

    static std::string loadProgram(std::string input);
    static cl::Device getGPUDevice();
};



#endif //AR_ENGINE_DISPARITY_H

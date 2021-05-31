//
// Created by magnus on 5/31/21.
//

#ifndef AR_ENGINE_FACEDETECTOR_H
#define AR_ENGINE_FACEDETECTOR_H


#include "opencv4/opencv2/opencv.hpp"

class FaceDetector {
public:

    FaceDetector();
    void detectFaceRegion(cv::Mat img);

private:

    cv::dnn::Net net;


};


#endif //AR_ENGINE_FACEDETECTOR_H

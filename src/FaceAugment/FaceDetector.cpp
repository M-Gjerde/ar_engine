//
// Created by magnus on 5/31/21.
//

#include "FaceDetector.h"

FaceDetector::FaceDetector() {

    std::string protoTxtFileName = "../user/NNM/deploy.prototxt.txt";
    std::string caffeModel = "../user/NNM/res10_300x300_ssd_iter_140000.caffemodel";

    net = cv::dnn::readNetFromCaffe(protoTxtFileName, caffeModel);

}

void FaceDetector::detectFaceRegion(cv::Mat img) {

    // Resize image to 300x300 res
    cv::resize(img, img, cv::Size(300, 300));

    cv::Mat blob = cv::dnn::blobFromImage(img, 1.0f, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0));
    cv::imshow("img",img);
    cv::waitKey(500);
    printf("[INFO] computing object detections...\n");
    net.setInput(blob, "data");
    cv::Mat detections = net.forward("detection_out");


    int k = 0;
}

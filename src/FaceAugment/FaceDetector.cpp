//
// Created by magnus on 5/31/21.
//

#include "FaceDetector.h"

FaceDetector::FaceDetector() {


}

void FaceDetector::detectFaceRegion(cv::Mat img) {
    const cv::Scalar meanVal(104.0, 177.0, 123.0);


    std::string protoTxtFileName = "../user/NNM/deploy.prototxt";
    std::string caffeModel = "../user/NNM/res10_300x300_ssd_iter_140000.caffemodel";

    cv::dnn::Net net = cv::dnn::readNetFromCaffe(protoTxtFileName, caffeModel);

    if (net.empty())
    {
        std::cerr << "Can't load network by using the following files: " << std::endl;
        std::cerr << "prototxt:   " << protoTxtFileName << std::endl;
        std::cerr << "caffemodel: " << caffeModel << std::endl;
        exit(-1);
    }

    // Resize image to 300x300 res
    cv::Mat imgResized;
    cv::Mat img2 = cv::imread("../user/NNM/bald_guys.jpg");
    img2 = cv::imread("../left.png");
    cv::resize(img, imgResized, cv::Size(300, 300));

    cv::Mat blob;
    cv::Mat inputBlob = cv::dnn::blobFromImage(imgResized, 1.0f,
                                  cv::Size(300, 300), meanVal, false, false); //Convert Mat to batch of images

    printf("[INFO] computing object detections...\n");
    net.setInput(inputBlob, "data");
    cv::Mat detections = net.forward("detection_out");
    printf("[INFO] COMPUTED...\n");
    cv::Mat detectionMat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());

    float confidenceThreshold = 0.2;
    for (int i = 0; i < detectionMat.rows; i++)
    {
        float confidence = detectionMat.at<float>(i, 2);

        if (confidence > confidenceThreshold)
        {
            int idx = static_cast<int>(detectionMat.at<float>(i, 1));
            int xLeftBottom = static_cast<int>(detectionMat.at<float>(i, 3) * img.cols);
            int yLeftBottom = static_cast<int>(detectionMat.at<float>(i, 4) * img.rows);
            int xRightTop = static_cast<int>(detectionMat.at<float>(i, 5) * img.cols);
            int yRightTop = static_cast<int>(detectionMat.at<float>(i, 6) * img.rows);

            cv::Rect object((int)xLeftBottom, (int)yLeftBottom,
                        (int)(xRightTop - xLeftBottom),
                        (int)(yRightTop - yLeftBottom));

            rectangle(img, object, cv::Scalar(0, 255, 0), 2);

        }
    }
    imshow("detections", img);
    cv::waitKey();


}

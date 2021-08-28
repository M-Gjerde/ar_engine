//
// Created by magnus on 5/31/21.
//

#ifndef AR_ENGINE_FACEDETECTOR_H
#define AR_ENGINE_FACEDETECTOR_H


#include <opencv4/opencv2/opencv.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv/cv_image.h>
#include <glm/vec3.hpp>

class FaceDetector {
public:

    FaceDetector();
    void detectFaceRegion(cv::Mat img);

    const cv::Rect &getRoiFace() const;

    bool isFaceFound() const;

    const glm::vec3 &getFaceRotVector() const;
    cv::Point2d getNosePoint2d();

private:
    cv::dnn::Net net;
    cv::Rect roiFace;

    bool foundFace = false;
    dlib::shape_predictor shapePredictor;
    glm::vec3 rotVec;
    cv::Point2d nosePoint2d;

};


#endif //AR_ENGINE_FACEDETECTOR_H

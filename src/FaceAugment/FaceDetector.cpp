//
// Created by magnus on 5/31/21.
//

#include "FaceDetector.h"

static dlib::rectangle openCVRectToDlib(cv::Rect r) {
    return dlib::rectangle((long) r.tl().x, (long) r.tl().y, (long) r.br().x - 1, (long) r.br().y - 1);
}


FaceDetector::FaceDetector() {
    std::string protoTxtFileName = "../user/NNM/deploy.prototxt";
    std::string caffeModel = "../user/NNM/res10_300x300_ssd_iter_140000.caffemodel";

    net = cv::dnn::readNetFromCaffe(protoTxtFileName, caffeModel);

    if (net.empty()) {
        std::cerr << "Can't load network by using the following files: " << std::endl;
        std::cerr << "prototxt:   " << protoTxtFileName << std::endl;
        std::cerr << "caffemodel: " << caffeModel << std::endl;
        exit(-1);
    }

    dlib::deserialize("../user/NNM/shape_predictor_68_face_landmarks.dat") >> shapePredictor;


}

    //cv::VideoCapture cap(6);

void FaceDetector::detectFaceRegion(cv::Mat img) {
    const cv::Scalar meanVal(123.0, 177.0, 123.0);

/*
    cv::Mat frame;
    cap >> frame;
    cv::imshow("Img cam", frame);
    if (frame.empty())
        return;
    img = frame;
*/
    // Resize image to 300x300 res
    cv::Mat imgResized;
    cv::resize(img, imgResized, cv::Size(300, 300));

    cv::Mat blob;
    cv::Mat inputBlob = cv::dnn::blobFromImage(imgResized, 1.0f,
                                               cv::Size(300, 300), meanVal, false,
                                               false); //Convert Mat to batch of images

    net.setInput(inputBlob, "data");
    cv::Mat detections = net.forward("detection_out");

    cv::Mat detectionMat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());
    foundFace = false;
    float confidenceThreshold = 0.5;
    for (int i = 0; i < detectionMat.rows; i++) {
        float confidence = detectionMat.at<float>(i, 2);

        if (confidence > confidenceThreshold) {
            int idx = static_cast<int>(detectionMat.at<float>(i, 1));
            int xLeftBottom = static_cast<int>(detectionMat.at<float>(i, 3) * img.cols);
            int yLeftBottom = static_cast<int>(detectionMat.at<float>(i, 4) * img.rows);
            int xRightTop = static_cast<int>(detectionMat.at<float>(i, 5) * img.cols);
            int yRightTop = static_cast<int>(detectionMat.at<float>(i, 6) * img.rows);

            roiFace = cv::Rect((int) xLeftBottom, (int) yLeftBottom,
                               (int) (xRightTop - xLeftBottom),
                               (int) (yRightTop - yLeftBottom));


            dlib::array2d<dlib::bgr_pixel> dlibImage;
            dlib::assign_image(dlibImage, dlib::cv_image<dlib::bgr_pixel>(img));


            auto rect = openCVRectToDlib(roiFace);
            dlib::full_object_detection shape = shapePredictor(dlibImage, rect);
            auto pos = shape.part(0);
            for (int j = 0; j < shape.num_parts(); ++j) {

                cv::circle(img, cv::Point(shape.part(j).x(), shape.part(j).y()), 1, cv::Scalar(255, 255, 255), 1);
            }

            cv::circle(img, cv::Point(shape.part(33).x(), shape.part(33).y()), 1, cv::Scalar(0, 0, 255), 1);
            cv::circle(img, cv::Point(shape.part(8).x(), shape.part(8).y()), 1, cv::Scalar(0, 0, 255), 1);
            cv::circle(img, cv::Point(shape.part(36).x(), shape.part(36).y()), 1, cv::Scalar(0, 0, 255), 1);
            cv::circle(img, cv::Point(shape.part(45).x(), shape.part(45).y()), 1, cv::Scalar(0, 0, 255), 1);
            cv::circle(img, cv::Point(shape.part(48).x(), shape.part(48).y()), 1, cv::Scalar(0, 0, 255), 1);
            cv::circle(img, cv::Point(shape.part(54).x(), shape.part(54).y()), 1, cv::Scalar(0, 0, 255), 1);


            rectangle(img, roiFace, cv::Scalar(0, 255, 0), 2);

            // POSE ESTIMATION
            // 3D model points for landmarks match.
            std::vector<cv::Point3d> model_points(68);
            std::ifstream in("../user/NNM/face_model.txt");
            std::string str;
            int line = 0;
            int coord = 0;
            cv::Point3d point;
            while (std::getline(in, str)) {
                if (!str.empty()) {
                   int index = line % 68;
                    if (line == 68) coord++;
                    if (line == 128) coord++;
                    switch (coord) {
                        case 0:
                            model_points[index].x = stof(str);
                            break;
                        case 1:
                            model_points[index].y = stof(str);
                            break;
                        case 2:
                            model_points[index].z = stof(str) * -1;  // Transform the model into a front view by multiplying -1
                            break;
                    }
                }
                line++;
            }

            std::vector<cv::Point2d> image_points;
            for (int j = 0; j < shape.num_parts(); ++j) {
                image_points.push_back(cv::Point2d(shape.part(j).x(), shape.part(j).y()));    // Nose tip

            }
            // Camera internals
            cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << 331.8015, 0, 314.2519, 0, 363.9482, 247.4047, 0, 0, 1);
            cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type); // Assuming no lens distortion

            // Output rotation and translation
            cv::Mat rotation_vector; // Rotation in axis-angle form
            cv::Mat translation_vector;

            // Solve for pose
            //cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rotation_vector, translation_vector);

            cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rotation_vector, translation_vector);


            // Project a 3D point (0, 0, 1000.0) onto the image plane.
            // We use this to draw a line sticking out of the nose

            std::vector<cv::Point3d> nose_end_point3D;
            std::vector<cv::Point2d> nose_end_point2D;
            //nose_end_point3D.push_back(cv::Point3d(0, 0, 1000.0));

            //projectPoints(nose_end_point3D, rotation_vector, translation_vector, camera_matrix, dist_coeffs, nose_end_point2D);
            //cv::line(img, image_points[33], nose_end_point2D[0], cv::Scalar(255, 0, 0), 2);

            cv::drawFrameAxes(img, camera_matrix, dist_coeffs, rotation_vector, translation_vector, 100);

            // Copy rotmat to glm vec
            rotVec.x = static_cast<float>(rotation_vector.at<double>(0, 0));
            rotVec.y = static_cast<float>(rotation_vector.at<double>(1, 0));
            rotVec.z = static_cast<float>(rotation_vector.at<double>(2, 0));
            // copy nose point for disparity usage
            nosePoint2d.x = shape.part(33).x();
            nosePoint2d.y = shape.part(33).y();
            foundFace = true;
        } else {

        }
    }
    //imshow("detections", img);
}

const cv::Rect &FaceDetector::getRoiFace() const {
    return roiFace;
}

cv::Point2d FaceDetector::getNosePoint2d() {
    return nosePoint2d;
}

const glm::vec3 &FaceDetector::getFaceRotVector() const {
    return rotVec;
}

bool FaceDetector::isFaceFound() const {
    return foundFace;
}

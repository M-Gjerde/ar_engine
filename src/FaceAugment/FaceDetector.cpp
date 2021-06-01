//
// Created by magnus on 5/31/21.
//

#include "FaceDetector.h"

static dlib::rectangle openCVRectToDlib(cv::Rect r)
{
    return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x - 1, (long)r.br().y - 1);
}


FaceDetector::FaceDetector() {
    std::string protoTxtFileName = "../user/NNM/deploy.prototxt";
    std::string caffeModel = "../user/NNM/res10_300x300_ssd_iter_140000.caffemodel";

    net = cv::dnn::readNetFromCaffe(protoTxtFileName, caffeModel);

    if (net.empty())
    {
        std::cerr << "Can't load network by using the following files: " << std::endl;
        std::cerr << "prototxt:   " << protoTxtFileName << std::endl;
        std::cerr << "caffemodel: " << caffeModel << std::endl;
        exit(-1);
    }

    dlib::deserialize("../user/NNM/shape_predictor_68_face_landmarks.dat") >> shapePredictor;


}

void FaceDetector::detectFaceRegion(cv::Mat img) {
    const cv::Scalar meanVal(104.0, 177.0, 123.0);


    // Resize image to 300x300 res
    cv::Mat imgResized;
    cv::resize(img, imgResized, cv::Size(300, 300));

    cv::Mat blob;
    cv::Mat inputBlob = cv::dnn::blobFromImage(imgResized, 1.0f,
                                  cv::Size(300, 300), meanVal, false, false); //Convert Mat to batch of images

    printf("[INFO] computing object detections...\n");
    net.setInput(inputBlob, "data");
    cv::Mat detections = net.forward("detection_out");
    printf("[INFO] COMPUTED...\n");
    cv::Mat detectionMat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());
    foundFace = false;
    float confidenceThreshold = 0.5;
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

            roiFace = cv::Rect((int)xLeftBottom, (int)yLeftBottom,
                        (int)(xRightTop - xLeftBottom),
                        (int)(yRightTop - yLeftBottom));



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
            // 3D model points.
            std::vector<cv::Point3d> model_points;
            model_points.push_back(cv::Point3d(0.0f, 0.0f, 0.0f));               // Nose tip
            model_points.push_back(cv::Point3d(0.0f, -330.0f, -65.0f));          // Chin
            model_points.push_back(cv::Point3d(-225.0f, 170.0f, -135.0f));       // Left eye left corner
            model_points.push_back(cv::Point3d(225.0f, 170.0f, -135.0f));        // Right eye right corner
            model_points.push_back(cv::Point3d(-150.0f, -150.0f, -125.0f));      // Left Mouth corner
            model_points.push_back(cv::Point3d(150.0f, -150.0f, -125.0f));       // Right mouth corner


            // 2D image points. If you change the image, you need to change vector
            std::vector<cv::Point2d> image_points;
            image_points.push_back( cv::Point2d(shape.part(33).x(), shape.part(33).y()));    // Nose tip
            image_points.push_back( cv::Point2d(shape.part(8).x(), shape.part(8).y()));    // Chin
            image_points.push_back( cv::Point2d(shape.part(36).x(), shape.part(36).y()));     // Left eye left corner
            image_points.push_back( cv::Point2d(shape.part(45).x(), shape.part(45).y()));    // Right eye right corner
            image_points.push_back( cv::Point2d(shape.part(48).x(), shape.part(48).y()));    // Left Mouth corner
            image_points.push_back( cv::Point2d(shape.part(54).x(), shape.part(54).y()));    // Right mouth corner

            // Camera internals
            cv::Mat camera_matrix = (cv::Mat_<double>(3,3) << 331.8015, 0, 314.2519, 0 , 363.9482, 247.4047, 0, 0, 1);
            cv::Mat dist_coeffs = cv::Mat::zeros(4,1,cv::DataType<double>::type); // Assuming no lens distortion

            // Output rotation and translation
            cv::Mat rotation_vector; // Rotation in axis-angle form
            cv::Mat translation_vector;

            // Solve for pose
            cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rotation_vector, translation_vector);

            // Project a 3D point (0, 0, 1000.0) onto the image plane.
            // We use this to draw a line sticking out of the nose

            std::vector<cv::Point3d> nose_end_point3D;
            std::vector<cv::Point2d> nose_end_point2D;
            nose_end_point3D.push_back(cv::Point3d(0,0,1000.0));

            projectPoints(nose_end_point3D, rotation_vector, translation_vector, camera_matrix, dist_coeffs, nose_end_point2D);
            cv::line(img,image_points[0], nose_end_point2D[0], cv::Scalar(255,0,0), 2);


            // Copy rotmat to glm vec
            rotVec.x = static_cast<float>(rotation_vector.at<double>(0, 0));
            rotVec.y = static_cast<float>(rotation_vector.at<double>(1, 0));
            rotVec.z = static_cast<float>(rotation_vector.at<double>(2, 0));
            foundFace = true;
        }
        else{

        }
    }
    imshow("detections", img);
}

const cv::Rect &FaceDetector::getRoiFace() const {
    return roiFace;
}

const glm::vec3 &FaceDetector::getFaceRotVector() const {
    return rotVec;
}

bool FaceDetector::isFaceFound() const {
    return foundFace;
}

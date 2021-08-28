//
// Created by magnus on 3/29/21.
//

#ifndef AR_ENGINE_HELPER_FUNCTIONS_H
#define AR_ENGINE_HELPER_FUNCTIONS_H


#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <libnoise/noise.h>

float getNoise(glm::vec3 coord){
    noise::module::Perlin myModule;

    float value = (float) myModule.GetValue (coord.x, coord.y, coord.z);

    return value;
}

void writePcdHeader(std::ofstream *file, std::string size) {
    *file << "# .PCD v0.7 - Point Cloud Data file format\n"
             "VERSION 0.7\n"
             "FIELDS x y z\n"
             "SIZE 4 4 4\n"
             "TYPE F F F\n"
             "COUNT 1 1 1\n"
             "WIDTH " + size + "\n"
                               "HEIGHT 1\n"
                               "VIEWPOINT 0 0 0 1 0 0 0\n"
                               "POINTS " + size + "\n"
                                                  "DATA ascii\n";

}

void disparityToPoint(float pixelValue, glm::vec3* point, cv::Point2d nosePoint) {
    float focalLength = 1.93;
    float baseline = 49.939;
    float pixelSize = 0.003;

    // OpenCV calibration values
    float fx = 331.8015, fy = 363.9482;
    float cx = 314.2519, cy = 247.4047;

    // Realsense factory calibrated values
    //double fx = 383.799, fy = 383.799;
    //double cx = 317.727, cy = 242.872;

    boost::numeric::ublas::matrix<float> K(4, 4);
    K(0, 0) = 1 / fx;
    K(0,1) = 0;
    K(0, 2) = (-cx * fx) / (fx * fy);
    K(0, 3) = 0;
    K(1, 0) = 0;
    K(1, 1) = 1 / fy;
    K(1, 2) = -cy / fy;
    K(1, 3) = 0;
    K(2, 0) = 0;
    K(2, 1) = 0;
    K(2, 2) = 1;
    K(2, 3) = 0;
    K(3, 0) = 0;
    K(3, 1) = 0;
    K(3, 2) = 0;
    K(3, 3) = 1;

    boost::numeric::ublas::vector<float> world(4);
    boost::numeric::ublas::vector<float> u(4);

    float disparity = (focalLength * baseline) / (pixelValue * pixelSize);
    u(0) = nosePoint.x;
    u(1) = nosePoint.y;
    u(2) = 1;
    u(3) = 1 / disparity;
    world = (float) 1 / 1000 * disparity * boost::numeric::ublas::prod(K, u);
    float x = world(0);
    float y = world(1);
    float z = world(2);
    point->x = x;
    point->y = y;
    point->z = z;

}

void disparityToPointWriteToFile(cv::Mat img, ArROI roi, std::vector<glm::vec3> *points) {
    float focalLength = 1.93;
    float baseline = 49.939;
    float pixelSize = 0.003;

    // OpenCV calibration values
    float fx = 331.8015, fy = 363.9482;
    float cx = 314.2519, cy = 247.4047;

    // Realsense factory calibrated values
    //double fx = 383.799, fy = 383.799;
    //double cx = 317.727, cy = 242.872;

    boost::numeric::ublas::matrix<float> K(4, 4);
    K(0, 0) = 1 / fx;
    K(0, 2) = (-cx * fx) / (fx * fy);
    K(1, 1) = 1 / fy;
    K(1, 2) = -cy / fy;
    K(2, 2) = 1;
    K(3, 3) = 1;

    boost::numeric::ublas::vector<float> world(4);

    std::ofstream outdata; // outdata is like cin
    outdata.open("../test_data.pcd", std::ios::trunc); // opens the file
    writePcdHeader(&outdata, std::to_string(roi.width * roi.height));
    boost::numeric::ublas::vector<float> u(4);

    float pixelValue, disparity;
    for (int i = roi.x; i < (roi.x + roi.width); ++i) {
        for (int j = roi.y; j < (roi.y + roi.height); ++j) {
            pixelValue = (float) img.at<float>(j, i) * 255;
            disparity = (focalLength * baseline) / (pixelValue * pixelSize);
            u(0) = i;
            u(1) = j;
            u(2) = 1;
            u(3) = 1 / disparity;
            world = (float) 1 / 1000 * disparity * boost::numeric::ublas::prod(K, u);
            float x = world(0);
            float y = world(1);
            float z = world(2);
            outdata << world(0) << " " << world(1) << " " << world(2) << std::endl;
            if (i == 320 && j == 400) {
                int k = 0;
            }
        }
    }

    outdata.close();
}

void createPointCloudWriteToPCD(cv::Mat img, std::string fileName) {

    float focalLength = 1.93;
    float baseline = 49.939;
    float pixelSize = 0.003;

    // OpenCV calibration values
    float fx = 331.8015, fy = 363.9482;
    float cx = 314.2519, cy = 247.4047;

    // Realsense factory calibrated values
    //double fx = 383.799, fy = 383.799;
    //double cx = 317.727, cy = 242.872;

    boost::numeric::ublas::matrix<float> K(4, 4);
    K(0, 0) = 1 / fx;
    K(0, 2) = (-cx * fx) / (fx * fy);
    K(1, 1) = 1 / fy;
    K(1, 2) = -cy / fy;
    K(2, 2) = 1;
    K(3, 3) = 1;

    boost::numeric::ublas::vector<float> world(4);

    std::ofstream outdata; // outdata is like cin
    outdata.open(fileName, std::ios::trunc); // opens the file
    //writePcdHeader(&outdata);
    boost::numeric::ublas::vector<float> u(4);

    float pixelValue, disparity;
    for (int i = 0; i < img.rows; ++i) {
        for (int j = 0; j < img.cols; ++j) {
            pixelValue = (float) img.at<float>(i, j) * 255;
            pixelValue = 128;
            disparity = (focalLength * baseline) / (pixelValue * pixelSize);
            u(0) = i;
            u(1) = j;
            u(2) = 1;
            u(3) = 1 / disparity;
            world = (float) 1 / 1000 * disparity * boost::numeric::ublas::prod(K, u);
            float x = world(0);
            float y = world(1);
            float z = world(2);

            outdata << world(0) << " " << world(1) << " " << world(2) << std::endl;
        }
    }

    outdata.close();
}



#endif //AR_ENGINE_HELPER_FUNCTIONS_H

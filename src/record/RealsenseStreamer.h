//
// Created by magnus on 3/28/21.
//

#ifndef AR_ENGINE_REALSENSESTREAMER_H
#define AR_ENGINE_REALSENSESTREAMER_H
#include <librealsense2/rs.hpp> // Include Intel RealSense Cross Platform API
#include <opencv2/core.hpp>


class RealsenseStreamer {
public:

    enum FrameType {
        IR_IMAGE_RIGHT = 0,
        IR_IMAGE_LEFT = 1,
        COLOR_IMAGE = 2,
        DEPTH_IMAGE = 3,
        COLOR_FRAME = 4,
        DEPTH_FRAME = 5
    };

    enum Commands {
        DISABLE = 0,
        ENABLE = 1
    };

    RealsenseStreamer(rs2::config config);
    void startStream();
    void setConfig(rs2::config config);
    bool streaming();
    void stopStream();
    void setEmitter(int type);

    ~RealsenseStreamer();


    cv::Mat getImage(int type) const;

    rs2::frame getFrame(int type) const;
    rs2::pipeline* pipe;
    rs2::pipeline_profile selection;

private:



    // Struct to store a set of frames from realsense
    struct Frame {
        rs2::frameset frameset;
        rs2::frame colorFrame;
        rs2::frame depthFrame;
        rs2::frame irFrameLeft;
        rs2::frame irFrameRight;

        uint depthHeight{};
        uint depthWidth{};
        uint colhHeight{};
        uint colWidth{};

        cv::Mat colorImage;
        cv::Mat irImageLeft;
        cv::Mat irImageRight;
        cv::Mat depthImage;

        uint32_t count = 0;
    };

    Frame frame;
    rs2::config cfg;

    bool streamStatus = false;

    void retrieveFrame();
};


#endif //AR_ENGINE_REALSENSESTREAMER_H

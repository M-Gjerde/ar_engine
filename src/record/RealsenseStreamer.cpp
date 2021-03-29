//
// Created by magnus on 3/28/21.
//

#include <iostream>
#include "RealsenseStreamer.h"

RealsenseStreamer::RealsenseStreamer(rs2::config config) {

    cfg = std::move(config);
    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    pipe = new rs2::pipeline();

}

void RealsenseStreamer::startStream() {
    // Configure and start the pipeline
    selection = pipe->start(cfg);
    streamStatus = true;

    // TODO CAN BE REMOVED - JUST DEBUGGING REALSENSE
    auto depth_stream = selection.get_stream(RS2_STREAM_DEPTH)
            .as<rs2::video_stream_profile>();
    auto resolution = std::make_pair(depth_stream.width(), depth_stream.height());
    auto i = depth_stream.get_intrinsics();
    auto principal_point = std::make_pair(i.ppx, i.ppy);
    auto focal_length = std::make_pair(i.fx, i.fy);
    rs2_distortion model = i.model;

    printf("principal x,y: %f, %f\n", principal_point.first, principal_point.second);
    printf("focal x,y: %f, %f\n", focal_length.first, focal_length.second);
    for (float coeff : i.coeffs) {
        printf("coeff: %f\n", coeff);
    }

}

void RealsenseStreamer::setEmitter(int cmd) {

    rs2::device selected_device = selection.get_device();
    auto dev = selected_device.first<rs2::depth_sensor>();

    if (dev.supports(RS2_OPTION_EMITTER_ENABLED)) {
        dev.set_option(RS2_OPTION_EMITTER_ENABLED, static_cast<float>(cmd));
    }

}

void RealsenseStreamer::setConfig(rs2::config config) {
    cfg = std::move(config);
}

void RealsenseStreamer::stopStream() {
    streamStatus = false;
    pipe->stop();
}

bool RealsenseStreamer::streaming() {
    retrieveFrame();

    return streamStatus;
}


void RealsenseStreamer::retrieveFrame() {
    cv::Mat image;
    frame.frameset = pipe->wait_for_frames();                               // Get a frameset from realsense pipeline object. This object holds all data from the realsense camera

    rs2::colorizer color_map;
    // Retrieve rs2 frames

    frame.depthFrame = frame.frameset.get_depth_frame().apply_filter(color_map);


    frame.irFrameLeft = frame.frameset.get_infrared_frame(1);
    frame.irFrameRight = frame.frameset.get_infrared_frame(2);

    /*
    frame.colorFrame = frame.frameset.get_color_frame();
    frame.colWidth = frame.colorFrame.as<rs2::video_frame>().get_width();    // Width for OpenCV Mat object
    frame.colhHeight = frame.colorFrame.as<rs2::video_frame>().get_height();  // Height for OpenCV Mat object
    // Put into OpenCV image objects
    frame.colorImage = cv::Mat(cv::Size(frame.colWidth, frame.colhHeight), CV_8UC3,
                               (void *) frame.colorFrame.get_data(),
                               cv::Mat::AUTO_STEP);
*/
    // Construct openCV mat object
    frame.irImageLeft = cv::Mat(cv::Size(frame.depthWidth, frame.depthHeight), CV_8UC1,
                                (void *) frame.irFrameLeft.get_data(),
                                cv::Mat::AUTO_STEP);                          // Construct openCV mat object
    frame.irImageRight = cv::Mat(cv::Size(frame.depthWidth, frame.depthHeight), CV_8UC1,
                                 (void *) frame.irFrameRight.get_data(), cv::Mat::AUTO_STEP);

    frame.depthFrame = frame.frameset.get_depth_frame();


    frame.depthWidth = frame.depthFrame.as<rs2::video_frame>().get_width();    // Width for OpenCV Mat object
    frame.depthHeight = frame.depthFrame.as<rs2::video_frame>().get_height();  // Height for OpenCV Mat object
    // Construct openCV mat object
    frame.depthImage = cv::Mat(cv::Size(frame.depthWidth, frame.depthHeight), CV_8UC3,
                               (void *) frame.depthFrame.get_data(), cv::Mat::AUTO_STEP);

    // Increment frame number
    frame.count++;
}

cv::Mat RealsenseStreamer::getImage(int type) const {

    switch (type) {
        case IR_IMAGE_LEFT:
            return frame.irImageLeft;
        case IR_IMAGE_RIGHT:
            return frame.irImageRight;
        case COLOR_IMAGE:
            return frame.colorImage;
        case DEPTH_IMAGE:
            return frame.depthImage;
        default:
            throw std::runtime_error("Image selection not supported");
    }

}

rs2::frame RealsenseStreamer::getFrame(int type) const {

    switch (type) {
        case DEPTH_FRAME:
            return frame.depthFrame;
        case COLOR_FRAME:
            return frame.colorFrame;
        default:
            throw std::runtime_error("Frame selection not supported");
    }
}


RealsenseStreamer::~RealsenseStreamer() = default;


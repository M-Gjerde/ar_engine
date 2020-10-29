#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by magnus on 10/8/20.
//

#include "Disparity.h"



int Disparity::init() {
    threadCheckInput = std::thread(&Disparity::checkForActions, this); // Create new thread to handle custom stuff


    run_status = true;
    return 0;
}

void Disparity::checkForActions() {
    std::cout << "New thread started\n";
    std::cout
            << "\nInput new integer:\n 1 = stop \n 2 = Get disparity OpenCL GPU device\n 3 = Get video disparity GPU device\n"
            << std::endl;

    std::cout << input << " <-- input" << std::endl;


    while (run_status) {
        switch (input) {
            case 1:
                run_status = false;
                input = 0;
                break;
            case 2:
                getDisparityFromImage(nullptr);
                input = 0;
                break;
            case 3:
                if (!disparityInProgress) {
                    disparityInProgress = true;
                    threadProcessDisparity = std::thread(&Disparity::getDisparityFromVideo, this);
                } else {
                    disparityInProgress = false;
                    pixelDataReady = false;
                    printf("Joining disparity process thread\n");
                    threadProcessDisparity.join();
                }
                input = 0;
                break;
        }
    }
}

void Disparity::cleanUp() {
    threadCheckInput.join();
    std::cout << "Disparity thread joined. Goodbye!" << std::endl;
}

cl::Device Disparity::getGPUDevice() {
    // Get a vector of available platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    // Get platform names
    for (const auto &i : platforms) {
        printf("Platform: %s\n", i.getInfo<CL_PLATFORM_NAME>().c_str());
    }
    auto platform = platforms.front();

    // Display devices
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    // Print some info about our devices
    for (const auto &i : devices) {
        printf("Device: %s\n", i.getInfo<CL_DEVICE_NAME>().c_str());
        printf("Device max work group size: %lu\n", i.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
        printf("Device max work item size: %lu\n", i.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0]);
        printf("Device max image height: %lu\n", i.getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>());
        printf("Device max image width: %lu\n", i.getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>());
        printf("Device max compute units: %u\n", i.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
        printf("Device max compute units: %lu\n", i.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>());


    }

    return devices.front();
}


void Disparity::getDisparityFromVideo() {

    // ----- IMAGES SETUP -----
     int numberOfImages = 339;
    int imageLoop = 0;
    std::string imageLeftPath = "/home/magnus/common/2011_09_26_drive_0091_sync/2011_09_26/2011_09_26_drive_0091_sync/image_00/data/";
    std::string imageRightPath = "/home/magnus/common/2011_09_26_drive_0091_sync/2011_09_26/2011_09_26_drive_0091_sync/image_01/data/";
    std::string imageNoPath;
    int width, height, imgSize;

    // ----- AMD GPU SETUP -----
    auto device = getGPUDevice();
    cl::Context context(device);
    cl::CommandQueue queue(context, device);

    // Create program
    cl::Program program = cl::Program(context, loadProgram("../kernels/disparity_video_kernel.cpp"), true);

    // --- Create Image Objects ---
    // Input image buffer data
    auto *leftImageData = new uchar[500000];
    auto *rightImageData = new uchar[500000];

    cl::ImageFormat imageFormat(CL_R, CL_UNSIGNED_INT8);

    // Read region and origin
    std::array<long unsigned int, 3> origin{};
    std::array<long unsigned int, 3> region{};

    pixelData = new unsigned char[465750];  // TODO pass imageSize

    // Output image buffer
    auto *imageResult = new uchar[465750];  // TODO pass imageSize


    while (disparityInProgress) {
        clock_t Start = clock();

        if (imageLoop == numberOfImages + 1) imageLoop = 0; // Stay in range of 339 images

        if (imageLoop < 10)
            imageNoPath = "000000000" + std::to_string(imageLoop) + ".png";
        else if (imageLoop < 99 && imageLoop > 9)
            imageNoPath = "00000000" + std::to_string(imageLoop) + ".png";
        else if (imageLoop > 99) {
            imageNoPath = "0000000" + std::to_string(imageLoop) + ".png";
        }

        cv::Mat imageLeft = cv::imread(imageLeftPath + imageNoPath, cv::IMREAD_GRAYSCALE);
        cv::Mat imageRight = cv::imread(imageRightPath + imageNoPath, cv::IMREAD_GRAYSCALE);

        // Create vectors of the images
        //std::vector<uchar> inputImgRightData(imgSize);
        cv::Mat flat = imageLeft.reshape(1, imageLeft.total() * imageLeft.channels());
        std::vector<uchar> inputImgLeftData = imageLeft.isContinuous() ? flat : flat.clone();

        cv::Mat flat2 = imageRight.reshape(1, imageRight.total() * imageRight.channels());
        std::vector<uchar> inputImgRightData = imageRight.isContinuous() ? flat2 : flat2.clone();

        height = imageLeft.rows;
        width = imageLeft.cols;
        imgSize = width * height; // default size with full size image is 352000

        // --- Create Image Objects ---
        // Input image buffer data
        leftImageData = inputImgLeftData.data();
        rightImageData = inputImgRightData.data();

        // Read origin and region for output buffer
        origin = {0, 0, 0};
        region = {static_cast<unsigned long>(width), static_cast<unsigned long>(height), 1};

        // Create output image
        cl::Image2D outImage(context, CL_MEM_WRITE_ONLY, imageFormat, width, height, 0, nullptr, nullptr);


        // Create input image
        cl::Image2D leftImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, width, height, 0,
                              leftImageData, nullptr);

        // Create input image
        cl::Image2D rightImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, width, height, 0,
                               rightImageData, nullptr);

        // --- Create Kernel ---
        cl::Kernel kernel = cl::Kernel(program, "image_kernel");

        kernel.setArg(0, leftImage);
        kernel.setArg(1, rightImage);
        kernel.setArg(2, outImage);

        cl_int result = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height),
                                                   cl::NDRange(256, 1));
        if (result != CL_SUCCESS)
            std::cerr << "util::getErrorString(result) " << std::endl;

        queue.finish();
        queue.enqueueReadImage(outImage, CL_TRUE, origin, region, width, 0, imageResult, nullptr, nullptr);


        memcpy(pixelData, imageResult, imgSize); // Putting data in pixelData handle

        pixelDataReady = true;


        auto endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
        printf("Time taken: %.7fs\n", endTime);

        /*
        cv::Mat outputImage(cv::Size(width, height), CV_8U);
        outputImage.data = pixelData;

        cv::normalize(outputImage, outputImage, 0, 255, cv::NORM_MINMAX, CV_8U);

        cv::namedWindow("Disparity image", cv::WINDOW_FULLSCREEN);
        cv::imshow("Disparity image", outputImage);

        int key = cv::waitKey(27);
        if (key == 27 || imageLoop == numberOfImages) {
            cv::destroyWindow("Disparity image");
            break;
        }
*/

        imageLoop++; // Increment to next image
    }
}

void Disparity::getDisparityFromImage(unsigned char **data) {
    // ----- Image loading -----
    // Specify images
    // TODO: Temporary method
    cv::Mat imageLeft = cv::imread("/home/magnus/CLionProjects/Stereovision/util/images/Aloe/view1.png",
                                   cv::IMREAD_GRAYSCALE);
    cv::Mat imageRight = cv::imread("/home/magnus/CLionProjects/Stereovision/util/images/Aloe/view5.png",
                                    cv::IMREAD_GRAYSCALE);


    cv::resize(imageLeft, imageLeft, cv::Size(640, 550), cv::INTER_NEAREST);
    cv::resize(imageRight, imageRight, cv::Size(640, 550), cv::INTER_NEAREST);

    // Write both images for manual inspection
    cv::imwrite("../../disparityResultLeft.png", imageLeft);
    cv::imwrite("../../disparityResultRight.png", imageRight);


    // Height of 550 px;
    int height = imageLeft.rows;
    // Width of 640 px
    int width = imageLeft.cols;
    int imgSize = width * height; // default size with full size image is 352000

    // Create vectors of the images
    //std::vector<uchar> inputImgRightData(imgSize);
    cv::Mat flat = imageLeft.reshape(1, imageLeft.total() * imageLeft.channels());
    std::vector<uchar> inputImgLeftData = imageLeft.isContinuous() ? flat : flat.clone();

    cv::Mat flat2 = imageRight.reshape(1, imageRight.total() * imageRight.channels());
    std::vector<uchar> inputImgRightData = imageRight.isContinuous() ? flat2 : flat2.clone();


    // ----- AMDGPU KERNEL SETUP -----
    // Get available platforms

    auto device = getGPUDevice();
    cl::Context context(device);
    cl::CommandQueue queue(context, device);

    // Create program
    cl::Program program = cl::Program(context, loadProgram("../kernels/disparity_image_kernel.cpp"), true);


    // --- Create Image Objects ---
    // Input image buffer data
    auto *leftImageData = new uchar[imgSize];
    leftImageData = inputImgLeftData.data();

    auto *rightImageData = new uchar[imgSize];
    rightImageData = inputImgRightData.data();

    // Output image buffer
    auto *imageResult = new uchar[imgSize];
    memset(imageResult, 0, imgSize);
    // Read origin and region for output buffer
    const std::array<long unsigned int, 3> origin = {0, 0, 0};
    const std::array<long unsigned int, 3> region = {static_cast<unsigned long>(width),
                                                     static_cast<unsigned long>(height), 1};

    cl::ImageFormat imageFormat(CL_R, CL_UNSIGNED_INT8);

    // Create output image
    cl::Image2D outImage(context, CL_MEM_WRITE_ONLY, imageFormat, width, height, 0, nullptr, nullptr);


    clock_t Start = clock();

    // Create input image
    cl::Image2D leftImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, width, height, 0,
                          leftImageData, nullptr);

    // Create input image
    cl::Image2D rightImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, width, height, 0,
                           rightImageData, nullptr);

    // --- Create Kernel ---
    cl::Kernel kernel = cl::Kernel(program, "image_kernel");

    kernel.setArg(0, leftImage);
    kernel.setArg(1, rightImage);
    kernel.setArg(2, outImage);


    /* Submitting "height" queues to the GPU to process each row method
    for(int i = 0; i < height; i++){
        queue.enqueueNDRangeKernel(kernel, cl::NDRange(0, i), cl::NDRange(width, 1), cl::NullRange);

    }
     */

    // Execute Kernel and copy device memory to host
    // 1. Kernel
    // 2. Offset
    // 3. Work area
    // 4. Work group sizes, Max work group size is 256
    cl_int result = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height), cl::NDRange(256, 1));
    if (result != CL_SUCCESS)
        std::cerr << "util::getErrorString(result)" << std::endl;

    queue.finish();
    queue.enqueueReadImage(outImage, CL_TRUE, origin, region, width, 0, imageResult, nullptr, nullptr);

    printf("Time taken: %.7fs\n", (double) (clock() - Start) / CLOCKS_PER_SEC);

    //data = new unsigned char[imgSize];
    imageSize = imgSize;
    imageHeight = height;
    imageWidth = width;
    memcpy(*data, imageResult, imgSize);

/*
    cv::Mat outputImage(cv::Size(width, height), CV_8U);
    outputImage.data = data;

    cv::normalize(outputImage, outputImage, 0, 255, cv::NORM_MINMAX, CV_8U);


      cv::namedWindow("Disparity image", cv::WINDOW_NORMAL);
      cv::imshow("Disparity image", outputImage);
      cv::imwrite("../../GPUResult.png", outputImage);



      while (true) {
          int key = cv::waitKey(27);
          if (key == 27) {
              cv::destroyWindow("Disparity image");
              break;
          }
      }
    */
    //clock_t Start = clock();
    //printf("Time taken: %.7fs\n", (double) (clock() - Start) / CLOCKS_PER_SEC);
}


std::string Disparity::loadProgram(std::string strInput) {
    std::ifstream stream(strInput.c_str());
    if (!stream.is_open()) {
        std::cout << "Cannot open file: " << strInput << std::endl;
        exit(1);
    }

    return std::string(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

}

void Disparity::stopProgram() {
    disparityInProgress = false;
    run_status = false;
}


#pragma clang diagnostic pop
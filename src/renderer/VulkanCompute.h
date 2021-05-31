//
// Created by magnus on 1/6/21.
//

#ifndef AR_ENGINE_VULKANCOMPUTE_H
#define AR_ENGINE_VULKANCOMPUTE_H

#include "../include/structs.h"
#include "../pipeline/Buffer.h"
#include "../pipeline/Descriptors.h"
#include "../Platform/Platform.h"
#include "../include/stb_image.h"
#include "../FaceAugment/ThreadSpawner.h"

class VulkanCompute {

public:
    VulkanCompute(ArEngine arEngine);

    ~VulkanCompute() = default;

    void setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, Platform *pPlatform, Pipeline pipeline);

    void loadComputeData();
    void loadComputeData(cv::Mat *rImg);

    void stopDisparityStream();

    void startDisparityStream();

    void cleanup();

    void loadImagePreviewData(ArCompute arCompute, Buffer *pBuffer) const;

    void executeComputeCommandBuffer();

    void readComputeResult();

private:

    ArEngine arEngine;
    ArDescriptor arDescriptor{};
    VkCommandPool commandPool{};
    ArPipeline computePipeline{};
    VkFence computeFence{};
    ArCompute arCompute;

    ThreadSpawner threadSpawner;
    ArSharedMemory *memP;

    ArROI ROI;
    cv::CascadeClassifier classifier;

    void setupFaceDetector();

};


#endif //AR_ENGINE_VULKANCOMPUTE_H

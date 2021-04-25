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
#include "../record/ThreadSpawner.h"

class VulkanCompute {

public:
    VulkanCompute(ArEngine arEngine);

    ~VulkanCompute() = default;

    ArCompute setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, Platform *pPlatform, Pipeline pipeline);
    void loadComputeData(ArCompute arCompute, Buffer *pBuffer);
    void stopDisparityStream();
    void startDisparityStream();
    void cleanup();

    void previewVideoStreams();

    const glm::vec4 &getRoi() const;
    void setRoi(const glm::vec4 &roi);
    void loadImagePreviewData(ArCompute arCompute, Buffer *pBuffer) const;

    bool takePhoto = false;



private:

    ArEngine arEngine;
    ArDescriptor arDescriptor{};
    VkCommandPool commandPool{};
    ArPipeline computePipeline{};

    ThreadSpawner threadSpawner;
    ArSharedMemory * memP;

    glm::vec4 ROI;
    cv::CascadeClassifier classifier;

    void setupFaceDetector();
};


#endif //AR_ENGINE_VULKANCOMPUTE_H

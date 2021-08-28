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

    void setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, ar::Platform *pPlatform, Pipeline pipeline);

    void loadComputeData(glm::vec4 roi, ArSharedMemory *memP);

    void stopDisparityStream();

    void startDisparityStream();

    void cleanup();

    void loadImagePreviewData(ArCompute arCompute, Buffer *pBuffer) const;

    void executeComputeCommandBuffer();

    cv::Mat readComputeResult();

private:

    ArEngine arEngine;
    ArDescriptor arDescriptor{};
    VkCommandPool commandPool{};
    ArPipeline computePipeline{};
    VkFence computeFence{};
    ArCompute arCompute;

};


#endif //AR_ENGINE_VULKANCOMPUTE_H

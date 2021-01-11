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

class VulkanCompute {

public:
    VulkanCompute(ArEngine arEngine);

    ~VulkanCompute() = default;

    ArCompute setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, Platform *pPlatform, Pipeline pipeline);
    void loadComputeData(ArCompute arCompute);
    void cleanup();

private:

    ArEngine arEngine;
    ArDescriptor arDescriptor{};
    VkCommandPool commandPool{};
    ArPipeline computePipeline{};

    void loadImageData();

};


#endif //AR_ENGINE_VULKANCOMPUTE_H

//
// Created by magnus on 10/13/20.
//

#ifndef AR_ENGINE_TEXTURES_H
#define AR_ENGINE_TEXTURES_H


#include "Images.h"
//#include "../stereo/Disparity.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstring>

class Textures : Images {
public:
    explicit Textures(Images *pImages);
    void createTexture(std::string fileName, ArTextureImage *pArTextureSampler, ArBuffer *imageStagingBuffer);

    void cleanUp(ArTextureImage arTextureSampler, ArBuffer imageBuffer);
    void setDisparityImageTexture(ArTextureImage *arTextureSampler, ArBuffer *imageBuffer);
    void createTextureImage(ArTextureImage *arTexture, ArBuffer *textureBuffer);
    void setDisparityVideoTexture(ArTextureImage *videoTexture, ArBuffer *imageBuffer);

private:
    Images *images;
    VkFormat format;

    void createTextureImage(std::string fileName, ArTextureImage *arTextureSampler, ArBuffer *imageBuffer);
    void createTextureSampler(ArTextureImage *arTextureSampler);
    void createTextureImageView(ArTextureImage *arTextureSampler);


};


#endif //AR_ENGINE_TEXTURES_H

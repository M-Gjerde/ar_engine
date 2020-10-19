//
// Created by magnus on 10/13/20.
//

#ifndef AR_ENGINE_TEXTURES_H
#define AR_ENGINE_TEXTURES_H


#include "Images.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstring>

class Textures : Images {
public:
    explicit Textures(Images *pImages);
    void createTexture(ArTextureSampler *pArTextureSampler, std::string fileName);

    void cleanUp();

private:

    ArBuffer imageBuffer;
    ArTextureSampler arTextureSampler;
    Images *images;
    VkFormat format;

    void createTextureImage(std::string fileName);
    void createTextureSampler();
    void createTextureImageView();

};


#endif //AR_ENGINE_TEXTURES_H

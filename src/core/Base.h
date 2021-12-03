//
// Created by magnus on 11/27/21.
//

#ifndef AR_ENGINE_BASE_H
#define AR_ENGINE_BASE_H

#include <ar_engine/src/core/vkMyModel.h>
#include "ar_engine/src/imgui/UISettings.h"
class Base {
public:

    struct SetupVars {
        VulkanDevice* device{};
        UISettings* ui{};
    };
    virtual ~Base() = default;

    virtual void update() = 0;
    virtual void setup(SetupVars vars) = 0;
    virtual void onUIUpdate(UISettings uiSettings) = 0;
    virtual std::string getType() {return type;}
    virtual vkMyModel getSceneObject() { return {};}

    std::string type = "None";
};

#endif //AR_ENGINE_BASE_H

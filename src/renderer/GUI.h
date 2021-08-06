//
// Created by magnus on 8/2/21.
//

#ifndef AR_ENGINE_GUI_H
#define AR_ENGINE_GUI_H

#include <vulkan/vulkan_core.h>
#include "../../external/imgui/imgui.h"


class GUI {

    explicit GUI(VkDevice device);
    ~GUI();

private:
    // Options and values to display/toggle from the UI
    struct UISettings {
        bool displayModels = true;
        bool displayLogos = true;
        bool displayBackground = true;
        bool animateLight = false;
        float lightSpeed = 0.25f;
        std::array<float, 50> frameTimes{};
        float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
        float lightTimer = 0.0f;
    } uiSettings;


    VkDevice mainDevice;

    void init(int width, int height);

    void initResources();

    void newFrame(bool updateFrameGraph);
};


#endif //AR_ENGINE_GUI_H

//
// Created by magnus on 9/23/21.
//

#ifndef AR_ENGINE_UISETTINGS_H
#define AR_ENGINE_UISETTINGS_H


#include <utility>
#include <vector>
#include <string>
#include <array>

struct UISettings {

public:

    struct intSlider {
        std::string name;
        int lowRange{};
        int highRange{};
        int val{};
    };

    bool rotate = true;
    bool displayLogos = true;
    bool displayBackground = true;
    bool toggleGridSize = false;
    float lightSpeed = 0.08;
    std::array<float, 50> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
    float lightTimer = 0.0f;

    std::vector<std::string> listBoxNames;
    uint32_t selectedListboxIndex = 0;

    void insertListboxItem(const std::string &item) {
        listBoxNames.push_back(item);
    }

    uint32_t getSelectedItem() {
        return selectedListboxIndex;
    }


    std::vector<intSlider *> intSliders;

    void createIntSlider(intSlider *slider) {
        intSliders.emplace_back(slider);
    }

};


#endif //AR_ENGINE_UISETTINGS_H

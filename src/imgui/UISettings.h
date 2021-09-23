//
// Created by magnus on 9/23/21.
//

#ifndef AR_ENGINE_UISETTINGS_H
#define AR_ENGINE_UISETTINGS_H


#include <vector>
#include <string>
#include <array>

struct UISettings {

public:
        bool displayModels = true;
        bool displayLogos = true;
        bool displayBackground = true;
        bool animateLight = false;
        float lightSpeed = 0.25f;
        std::array<float, 50> frameTimes{};
        float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
        float lightTimer = 0.0f;

        std::vector<std::string> listBoxNames;
        uint32_t selectedListboxIndex = -1;

        void insertListboxItem(const std::string& item){
            listBoxNames.push_back(item);
        }

        uint32_t getSelectedItem(){
            return selectedListboxIndex;
        }



};


#endif //AR_ENGINE_UISETTINGS_H

//
// Created by magnus on 11/1/20.
//

#ifndef AR_ENGINE_LOADSETTINGS_H
#define AR_ENGINE_LOADSETTINGS_H

#include <nlohmann/json.hpp>
#include <iostream>
#include "../renderer/VulkanRenderer.hpp"

class LoadSettings {


private:
    nlohmann::json json;

    std::vector<std::map<std::string, std::string>> sceneObjects;
public:
    void init();
    void saveScene(VulkanRenderer vulkanRenderer);
    const std::vector<std::map<std::string, std::string>> &getSceneObjects() const;
};


#endif //AR_ENGINE_LOADSETTINGS_H

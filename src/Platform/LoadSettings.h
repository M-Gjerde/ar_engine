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
    std::string fPath;
    std::vector<std::map<std::string, std::string>> sceneObjects;
public:

    LoadSettings(std::string file);

    void init();
    void saveScene(const VulkanRenderer& vulkanRenderer);
    [[nodiscard]] const std::vector<std::map<std::string, std::string>> &getSceneObjects() const;
};


#endif //AR_ENGINE_LOADSETTINGS_H

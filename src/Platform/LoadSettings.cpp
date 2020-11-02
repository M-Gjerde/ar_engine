//
// Created by magnus on 11/1/20.
//

#include <fstream>
#include "LoadSettings.h"

void LoadSettings::init() {
    nlohmann::json json;

    // read a JSON file
    std::ifstream jsonStream("../user/config.json");
    // Put into JSON object
    jsonStream >> json;

    int numberOfDifferentObjects = 3;
    sceneObjects.resize(numberOfDifferentObjects); // Create a map containing different objects


    // even easier with structured bindings (C++17)
    for (auto&[iKey, iVal] : json.items()) {
        if (iKey == "settings") {
            for (auto&[jKey, jVal] : iVal.items()) {
                if (jKey == "objects") {

                    // Number of different objects in the scene
                    for (int i = 0; i < jVal.size(); ++i) {
                        for (auto&[key, value] : jVal[i].items()) {
                            sceneObjects[i].insert(std::make_pair(key.data(), value.get<std::string>()));
                        }
                    }
                }
            }
        }
    }


    printf("Finished reading scene settings\n");
}

const std::vector<std::map<std::string, std::string>> &LoadSettings::getSceneObjects() const {
    return sceneObjects;
}

//
// Created by magnus on 11/1/20.
//


#include "LoadSettings.h"


LoadSettings::LoadSettings(std::string file) {
    fPath = std::move(file);
    init();
}

void LoadSettings::init() {

    // read a JSON file
    std::ifstream jsonStream("../user/"+fPath+".json");

    // Put into JSON object
    jsonStream >> json;

    size_t numberOfDifferentObjects = json["settings"]["objects"].size();

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

    // Indexing specific elements
    //json["settings"]["objects"][0]["posX"] = "-10";

}

const std::vector<std::map<std::string, std::string>> &LoadSettings::getSceneObjects() const {
    return sceneObjects;
}

/*
void LoadSettings::saveScene(const VulkanRenderer& vulkanRenderer) {
    std::vector<SceneObject> objects = vulkanRenderer.getSceneObjects();

    for (int i = 0; i < objects.size(); ++i) {
        glm::mat4 model = objects[i].getModel();
        // Save positions (Last column of model matrix)
        json["settings"]["objects"][i]["posX"] = std::to_string(model[3].x);
        json["settings"]["objects"][i]["posY"] = std::to_string(model[3].y);
        json["settings"]["objects"][i]["posZ"] = std::to_string(model[3].z);

        // TODO Save scaling and rotation too.

    }

    std::ofstream file("../user/config.json");
    file << json;

}

*/
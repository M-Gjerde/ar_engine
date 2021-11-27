//
// Created by magnus on 9/6/21.
//

#ifndef AR_ENGINE_TERRAIN_H
#define AR_ENGINE_TERRAIN_H


#include <ar_engine/src/core/ScriptBuilder.h>
#include <ar_engine/src/core/vkMyModel.h>

class Terrain : public Base, public RegisteredInFactory<Terrain>, vkMyModel {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Terrain() {
        s_bRegistered;
    }
    /** @brief Static method to create class, returns a unique ptr of Terrain **/
    static std::unique_ptr<Base> CreateMethod() { return std::make_unique<Terrain>(); }
    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Terrain"; }

    /** @brief Setup function called one during engine prepare **/
    void setup() override;
    /** @brief update function called once per frame **/
    void update() override;
    /** @brief Initialize function **/
    void initialize(VulkanDevice* device) override;
    /** @brief draw function **/
    void draw(VkCommandBuffer commandBuffer) override;
    /** @brief Get the type of script. This will determine how it interacts with a gameobject **/
    std::string getType() override;

    void generateSquare();

    /** @brief public string to determine if this script should be attaced to an object,
     * create a new object or do nothing. Types: Generator | None | Name of object in object folder **/
    std::string type = "generator";

    void setSceneObject(SceneObject *_sceneObject) override;
    SceneObject getSceneObject() override;
    SceneObject* sceneObject;
};


#endif //AR_ENGINE_TERRAIN_H

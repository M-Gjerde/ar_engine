//
// Created by magnus on 1/12/22.
//

#ifndef AR_ENGINE_CUBE_H
#define AR_ENGINE_CUBE_H


#include <ar_engine/src/core/ScriptBuilder.h>
#include <ar_engine/src/core/glTFModel.h>

/** @brief Example class for implementing a script. Remember to include ScriptBuilder.h **/
class Cube : public Base, public RegisteredInFactory<Cube>, glTFModel {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Cube() {
        s_bRegistered;
    }

    /** @brief Static method to create class, returns a unique ptr of Example **/
    static std::unique_ptr<Base> CreateMethod() { return std::make_unique<Cube>(); }

    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Cube"; }

    /** @brief Setup function called one during engine prepare **/
    void setup(SetupVars vars) override;

    /** @brief update function called once per frame **/
    void update() override;

    /** @brief update function called once per frame **/
    void onUIUpdate(UISettings uiSettings) override;

    /** @brief Get the type of script. This will determine how it interacts with a gameobject **/
    std::string getType() override;

    /** @brief public string to determine if this script should be attaced to an object,
    * create a new object or do nothing. Types: Generator | None | Name of object in object folder **/
    std::string type = "Render";

    void *selection = (void *) "0";

    void prepareObject() override;

    void updateUniformBufferData(uint32_t index, void *params, void *matrix, Camera* camera) override;

    void draw(VkCommandBuffer commandBuffer, uint32_t i) override;

};



#endif //AR_ENGINE_CUBE_H

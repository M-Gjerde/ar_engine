//
// Created by magnus on 1/12/22.
//

#ifndef AR_ENGINE_SUN_H
#define AR_ENGINE_SUN_H


#include <ar_engine/src/core/ScriptBuilder.h>
#include <ar_engine/src/core/glTFModel.h>

/** @brief Example class for implementing a script. Remember to include ScriptBuilder.h **/
class Sun : public Base, public RegisteredInFactory<Sun>, glTFModel {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Sun() {
        s_bRegistered;
    }

    /** @brief Static method to create class, returns a unique ptr of Example **/
    static std::unique_ptr<Base> CreateMethod() { return std::make_unique<Sun>(); }

    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Sun"; }

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
    UBOMatrix mat;
    float rotation = 0.0f;

    void prepareObject() override;

    void draw(VkCommandBuffer commandBuffer, uint32_t i) override;

};



#endif //AR_ENGINE_SUN_H

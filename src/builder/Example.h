//
// Created by magnus on 9/5/21.
//

#ifndef AR_ENGINE_EXAMPLE_H
#define AR_ENGINE_EXAMPLE_H

#include <ar_engine/src/core/ScriptBuilder.h>

/** @brief Example class for implementing a script. Remember to include ScriptBuilder.h **/
class Example : public Base, public RegisteredInFactory<Example> {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Example() {
        s_bRegistered;
    }

    /** @brief Static method to create class, returns a unique ptr of Example **/
    static std::unique_ptr<Base> CreateMethod() { return std::make_unique<Example>(); }

    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Example"; }

    /** @brief Setup function called one during engine prepare **/
    void setup() override;

    /** @brief update function called once per frame **/
    void update() override;

    /** @brief Get the type of script. This will determine how it interacts with a gameobject **/
    std::string getType() override;

    /** @brief public string to determine if this script should be attaced to an object,
    * create a new object or do nothing. Types: Generator | None | Name of object in object folder **/
    std::string type = "None";

};


#endif //AR_ENGINE_EXAMPLE_H

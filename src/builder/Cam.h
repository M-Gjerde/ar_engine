//
// Created by magnus on 9/6/21.
//

#ifndef AR_ENGINE_CAM_H
#define AR_ENGINE_CAM_H

#include <ar_engine/src/core/ScriptBuilder.h>


/** @brief Example class for implementing a script. Remember to include ScriptBuilder.h **/
class Cam : public Base, public RegisteredInFactory<Cam> {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Cam() {
        s_bRegistered;
    }
    /** @brief Static method to create class, returns a unique ptr of Example **/
    static std::unique_ptr<Base> CreateMethod() { return std::make_unique<Cam>(); }
    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Cam"; }
    /** @brief Setup function called one during engine prepare **/
    void setup() override;
    /** @brief update function called once per frame **/
    void update() override;

};


#endif //AR_ENGINE_CAM_H

//
// Created by magnus on 9/6/21.
//

#ifndef AR_ENGINE_TERRAIN_H
#define AR_ENGINE_TERRAIN_H


#include <ar_engine/src/core/ScriptBuilder.h>

class Terrain : public Component, public RegisteredInFactory<Terrain> {

public:
    /** @brief Constructor. Just run s_bRegistered variable such that the class is
     * not discarded during compiler initialization. Using the power of static variables to ensure this **/
    Terrain() {
        s_bRegistered;
    }
    /** @brief Static method to create class, returns a unique ptr of Terrain inherits Component **/
    static std::unique_ptr<Component> CreateMethod() { return std::make_unique<Terrain>(); }
    /** @brief Name which is registered for this class. Same as ClassName **/
    static std::string GetFactoryName() { return "Terrain"; }
    /** @brief Setup function called one during engine prepare **/
    void setup() override;
    /** @brief update function called once per frame **/
    void update() override;

    void generateSquare();
};


#endif //AR_ENGINE_TERRAIN_H

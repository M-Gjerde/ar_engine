//
// Created by magnus on 9/6/21.
//

#ifndef AR_ENGINE_SCRIPTBUILDER_H
#define AR_ENGINE_SCRIPTBUILDER_H
#include <memory>
#include <map>
#include <iostream>
#include <vector>
#include <cassert>
#include <ar_engine/src/core/structs.h>

class SceneObject {
public:

    SceneObject() = default;
    ~SceneObject() = default;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

};

class Base {
public:

    virtual ~Base() = default;

    virtual void update() = 0;
    virtual void setup() = 0;

    SceneObject* sceneObject;
};


class ComponentMethodFactory {
public:
    using TCreateMethod = std::unique_ptr<Base>(*)();
    TCreateMethod m_CreateFunc;

public:
    ComponentMethodFactory() = delete;

    static bool Register(const std::string name, TCreateMethod createFunc) {
        if (auto it = s_methods.find(name); it == s_methods.end()) {
            s_methods[name] = createFunc;
            std::cout << name << " registered\n";
            return true;
        }
        return false;
    }

    static std::unique_ptr<Base> Create(const std::string &name) {
        if (auto it = s_methods.find(name); it != s_methods.end()){
            return it->second();
        }
        return nullptr;
    }


private:
    static std::map<std::string, TCreateMethod> s_methods;
};

inline std::map<std::string, ComponentMethodFactory::TCreateMethod> ComponentMethodFactory::s_methods;

template<typename T>
class RegisteredInFactory {
protected:
    static bool s_bRegistered;
};

template<typename T>
bool RegisteredInFactory<T>::s_bRegistered = ComponentMethodFactory::Register(T::GetFactoryName(), T::CreateMethod);




#endif //AR_ENGINE_SCRIPTBUILDER_H

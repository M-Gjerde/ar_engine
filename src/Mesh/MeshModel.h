//
// Created by magnus on 8/8/20.
//

#ifndef UDEMY_VULCAN_MESHMODEL_H
#define UDEMY_VULCAN_MESHMODEL_H


#include <assimp/scene.h>
#include "Mesh.hpp"

class MeshModel {

public:
    MeshModel();
    MeshModel(std::vector<Mesh> newMeshList);
    size_t getMeshCount();
    Mesh* getMesh(size_t index);

    glm::mat4 getModel();
    void setModel(glm::mat4 newModel);

    void destroyMeshModel();

    static std::vector<std::string> LoadMaterials(const aiScene* scene);
    static std::vector<Mesh> LoadNode(Utils::MainDevice mainDevice, VkQueue transferQueue,
                                      VkCommandPool transferCommandPool, aiNode * node, const aiScene* scene, std::vector<int> matToTex);

    static Mesh LoadMesh(Utils::MainDevice mainDevice, VkQueue transferQueue,
                         VkCommandPool transferCommandPool, aiMesh * mesh, const aiScene* scene, std::vector<int> matToTex);
    ~MeshModel();

private:
    std::vector<Mesh> meshList;
    glm::mat4 model{};

};


#endif //UDEMY_VULCAN_MESHMODEL_H

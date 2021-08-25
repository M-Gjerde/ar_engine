//
// Created by magnus on 8/15/21.
//

#ifndef AR_ENGINE_MESHGENERATOR_H
#define AR_ENGINE_MESHGENERATOR_H


#include <ar_engine/external/Perlin_Noise/ppm.h>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>

#include <utility>
#include "ar_engine/src/Models/SceneObject.h"

class MeshGenerator {
public:

    // Populate a list of settings
    std::vector<ArMeshInfoUI> settings;
    bool reload = false;
    unsigned int seed = 237;
    PerlinNoise* pn;

    MeshGenerator(ArEngine _arEngine, VkRenderPass _renderPass) {
        arEngine = std::move(_arEngine);
        renderPass = _renderPass;
        arModel.transferQueue = arEngine.graphicsQueue;
        arModel.transferCommandPool = arEngine.commandPool;

        pn = new PerlinNoise(seed);

        // Enable setting in DEBUG mode
        settings.resize(3);
        // Name, variable, min and max range
        // Slider Y-noise:
        settings[0].maxRange = 100.0f;
        settings[0].minRange = 0.0f;
        settings[0].name = "Y-Noise";
        settings[0].active = true;
        settings[0].type = "slider";
        settings[0].floatVal = 0.0f;
        // Grid size
        // Neex two inputs
        settings[1].type = "inputInt";
        settings[1].name = "xSize";
        settings[1].active = true;
        settings[1].intVal = 5;
        settings[2].type = "inputInt";
        settings[2].name = "zSize";
        settings[2].active = true;
        settings[2].intVal = 5;
    }

    ~MeshGenerator(){
        delete pn;
        delete terrain;
    }


    SceneObject createSceneObject(){

    }


    void update(std::vector<ArMeshInfoUI> _settings, ArModel* _arModel) {
        // Delete old mesh
        reload = true;
        // update setting
        settings = std::move(_settings);
        // recreate Mesh
        generateSquare();
        *_arModel = arModel;
    }

private:
    SceneObject *terrain{};

    ArEngine arEngine{};
    VkRenderPass renderPass{};
    ArShadersPath shadersPath{};
    ArModel arModel{};
    ArDescriptorInfo descriptorInfo{};

    void generateSquare() {
        // 16*16 mesh as our ground
        // Get square size from input
        int xSize = 12;
        int zSize = 12;

        for (const auto& setting: settings) {
            if (setting.active && setting.type == "inputInt") {
                xSize = settings[1].intVal;
                zSize = settings[2].intVal;

            }
        }
        int v = 0;

        arModel.vertexCount = (xSize + 1) * (zSize + 1);
        arModel.indexCount = xSize * zSize * 6;
        // Alloc memory for vertices and indices

        arModel.vertices.clear();
        arModel.indices.clear();

        arModel.vertices = std::vector<Vertex>(arModel.vertexCount);

        for (int z = 0; z <= zSize; ++z) {
            for (int x = 0; x <= xSize; ++x) {
                Vertex vertex{};

                // Use the grid size to determine the perlin noise image.
                double i = (double) x / ((double) xSize);
                double j = (double) z / ((double) zSize);
                double n = pn->noise(settings[0].floatVal * i, settings[0].floatVal * j, 0.8);
                vertex.pos = glm::vec3(x, n, z);
                arModel.vertices[v] = vertex;
                v++;

                // calculate Normals for every 3 vertices
                if ((v % 3) == 0) {
                    glm::vec3 A = arModel.vertices[v - 2].pos;
                    glm::vec3 B = arModel.vertices[v - 1].pos;
                    glm::vec3 C = arModel.vertices[v].pos;
                    glm::vec3 AB = B - A;
                    glm::vec3 AC = C - A;

                    glm::vec3 normal = glm::cross(AC, AB);
                    normal = glm::normalize(normal);
                    // Give normal to last three vertices
                    arModel.vertices[v - 2].normal = normal;
                    arModel.vertices[v - 1].normal = normal;
                    arModel.vertices[v].normal = normal;

                }
            }
        }

        arModel.indices = std::vector<uint32_t>(arModel.indexCount);
        int tris = 0;
        int vert = 0;
        for (int z = 0; z < zSize; ++z) {
            for (int x = 0; x < xSize; ++x) {
                // One quad
                arModel.indices[tris + 0] = vert;
                arModel.indices[tris + 1] = vert + 1;
                arModel.indices[tris + 2] = vert + xSize + 1;
                arModel.indices[tris + 3] = vert + 1;
                arModel.indices[tris + 4] = vert + xSize + 2;
                arModel.indices[tris + 5] = vert + xSize + 1;

                vert++;
                tris += 6;
            }
            vert++;
        }

    }


};


#endif //AR_ENGINE_MESHGENERATOR_H

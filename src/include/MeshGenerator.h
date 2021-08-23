//
// Created by magnus on 8/15/21.
//

#ifndef AR_ENGINE_MESHGENERATOR_H
#define AR_ENGINE_MESHGENERATOR_H


#include <ar_engine/external/Perlin_Noise/ppm.h>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>

class MeshGenerator {
public:

    // Populate a list of settings
    ArGuiSliderMeshGenerator settings;
    void onSettingsUpdate(ArGuiSliderMeshGenerator _settings){
        settings = _settings;
    }

    MeshGenerator(ArEngine _arEngine, VkRenderPass _renderPass) {
        arEngine = std::move(_arEngine);
        renderPass = _renderPass;
        arModel.transferQueue = arEngine.graphicsQueue;
        arModel.transferCommandPool = arEngine.commandPool;

        // Enable setting in DEBUG mode
        // For a slider:
        // Name, variable, min and max range
        settings.maxRange = 100.0f;
        settings.minRange = 0.0f;
        settings.name = "Y-Noise";
        settings.active = true;
        settings.type = "slider";


    }


    void prepareResources() {
        shadersPath.fragmentShader = "phongLightFrag";
        shadersPath.vertexShader = "defaultVert";

        // ModelInfo
        generateSquare();


        // DescriptorInfo
        // Create descriptor
        // Font uses a separate descriptor pool
        ArDescriptorInfo descriptorInfo{};
        // poolcount and descriptor set counts
        descriptorInfo.descriptorPoolCount = 1;
        descriptorInfo.descriptorCount = 2;
        descriptorInfo.descriptorSetLayoutCount = 2;
        descriptorInfo.descriptorSetCount = 2;
        std::vector<uint32_t> descriptorCounts;
        descriptorCounts.push_back(1);
        descriptorCounts.push_back(1);
        descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
        std::vector<uint32_t> bindings;
        bindings.push_back(0);
        bindings.push_back(1);
        descriptorInfo.pBindings = bindings.data();
        // dataSizes
        std::vector<uint32_t> dataSizes;
        dataSizes.push_back(sizeof(uboModel));
        dataSizes.push_back(sizeof(FragmentColor));
        descriptorInfo.dataSizes = dataSizes.data();
        // types
        std::vector<VkDescriptorType> types(2);
        types[0] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        types[1] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorInfo.pDescriptorType = types.data();
        // stages
        std::array<VkShaderStageFlags, 2> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
        descriptorInfo.stageFlags = stageFlags.data();

        terrain = new SceneObject(arEngine, shadersPath, arModel, descriptorInfo);
        terrain->createPipeline(renderPass);
    }

    SceneObject getSceneObject() {
        return *terrain;
    }

private:
    SceneObject *terrain{};
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

    ArEngine arEngine{};
    VkRenderPass renderPass{};
    ArShadersPath shadersPath{};
    ArModel arModel{};

    void generateSquare() {
        // 16*16 mesh as our ground
        const int xSize = 3;
        const int zSize = 3;
        // Create an ampty PPm image
        ppm image(xSize, zSize);
        unsigned int seed = 237;
        PerlinNoise pn(seed);


        int v = 0;
        arModel.vertices.resize((xSize+1) * (zSize + 1));
        for (int z = 0; z <= zSize; ++z) {
            for (int x = 0; x <= xSize; ++x) {
                Vertex vertex{};

                // Use the grid size to determine the perlin noise image.
                double i = (double) x / ((double)xSize);
                double j = (double) z / ((double)zSize);
                double n = pn.noise(settings.val * i, settings.val * j, 0.8);

                vertex.pos = glm::vec3(x, n, z);;

                arModel.vertices[v] = vertex;
                v++;

                // calculate Normals for every 3 vertices
                if ((v % 3) == 0){
                    glm::vec3 A =  arModel.vertices[v - 2].pos;
                    glm::vec3 B =  arModel.vertices[v - 1].pos;
                    glm::vec3 C =  arModel.vertices[v].pos;
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

        arModel.indices.resize(xSize * zSize * 6);
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

        arModel.indexCount = arModel.indices.size();
        arModel.vertexCount = arModel.vertices.size();
    }


};


#endif //AR_ENGINE_MESHGENERATOR_H

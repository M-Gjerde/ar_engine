//
// Created by magnus on 8/15/21.
//

#ifndef AR_ENGINE_MESHGENERATOR_H
#define AR_ENGINE_MESHGENERATOR_H


class MeshGenerator {
public:


    MeshGenerator(ArEngine _arEngine, VkRenderPass _renderPass) {
        arEngine = std::move(_arEngine);
        renderPass = _renderPass;


        arModel.transferQueue = arEngine.graphicsQueue;
        arModel.transferCommandPool = arEngine.commandPool;

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
        const int xSize = 17;
        const int zSize = 17;

        for (int z = 0; z < zSize + 1; ++z) {
            for (int x = 0; x < xSize + 1; ++x) {
                Vertex vertex{};
                vertex.pos = glm::vec3(x, 0, z);;
                vertex.normal = glm::vec3(0, 1, 0);
                arModel.vertices.push_back(vertex);

            }
        }

        int vert = 0;
        const size_t indexCount = xSize * zSize * 6 * 2;
        std::array<uint32_t, indexCount> triangles{};
        for (int z = 0; z < zSize + 1; ++z) {
            for (int x = 0; x < xSize + 1; ++x) {
                // One quad
                // triangle 1
                arModel.indices.push_back(vert);
                arModel.indices.push_back(vert + xSize + 1);
                arModel.indices.push_back(vert + 1);
                arModel.indices.push_back(vert + 1);
                arModel.indices.push_back(vert + xSize + 1);
                arModel.indices.push_back(vert + xSize + 2);

                vert++;
            }
        }

        arModel.indexCount = arModel.indices.size();
        arModel.vertexCount = arModel.vertices.size();
    }


};


#endif //AR_ENGINE_MESHGENERATOR_H

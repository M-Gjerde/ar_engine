//
// Created by magnus on 9/6/21.
//

#include <glm/glm.hpp>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>
#include "Terrain.h"


void Terrain::setup(SetupVars vars) {
    printf("Terrain setup\n");
    vulkanDevice = vars.device; // TODO No actual need for multiple VkDevice references
    b.renderPass = vars.renderPass;
    b.UBCount = vars.UBCount;
    b.device = vars.device;


    xSizeSlider.name = "xSize";
    xSizeSlider.lowRange = 0;
    xSizeSlider.highRange = 1000;
    xSizeSlider.val = 150;
    vars.ui->createIntSlider(&xSizeSlider);

    zSizeSlider.name = "zSize";
    zSizeSlider.lowRange = 0;
    zSizeSlider.highRange = 1000;
    zSizeSlider.val = 150;
    vars.ui->createIntSlider(&zSizeSlider);

    noise.name = "noise_multiplier";
    noise.lowRange = 0;
    noise.highRange = 200;
    noise.val = 3;
    vars.ui->createIntSlider(&noise);

    sinMod.name = "height";
    sinMod.lowRange = 0;
    sinMod.highRange = 200;
    sinMod.val = 50;
    vars.ui->createIntSlider(&sinMod);

    generateSquare();

}


void Terrain::generateSquare() {
    // 16*16 mesh as our ground
    // Get square size from input
    auto *pn = new PerlinNoise(123);


    uint32_t vertexCount = (xSizeSlider.val + 1) * (zSizeSlider.val + 1);
    auto *vertices = new MyModel::Model::Vertex[vertexCount + 1];
    uint32_t indexCount = xSizeSlider.val * zSizeSlider.val * 6;
    auto *indices = new uint32_t[indexCount + 1];

    uint32_t v = 0;
    // Alloc memory for vertices and indices
    for (int z = 0; z <= zSizeSlider.val; ++z) {
        for (int x = 0; x <= xSizeSlider.val; ++x) {
            MyModel::Model::Vertex vertex{};

            // Use the grid size to determine the perlin noise image.
            double i = (double) x / ((double) xSizeSlider.val);
            double j = (double) z / ((double) zSizeSlider.val);

            double height = sin(x + z);

            double n = noise.val * pn->noise(i * sinMod.val, j * sinMod.val, 1) ;//+ height * sinMod.val;
            //n = n - floor(n); Wood like structure

            double xPos = (double) x / 1;
            double zPos = (double) z / 1;

            vertex.pos = glm::vec3(xPos, n, zPos);
            vertices[v] = vertex;
            v++;
        }
    }


    // Normals
    int index = 0;
    int quad = 0;
    for (int z = 0; z < zSizeSlider.val; ++z) {
        for (int x = 0; x < xSizeSlider.val; ++x) {
            glm::vec3 A = vertices[index].pos;
            glm::vec3 B = vertices[index + 1].pos;
            glm::vec3 C = vertices[index + 1 + xSizeSlider.val].pos;
            // Normals and stuff
            glm::vec3 AB = B - A;
            glm::vec3 AC = C - A;
            glm::vec3 normal = glm::cross(AC, AB);
            normal = glm::normalize(normal);
            // Give normal to last three vertices
            vertices[index].normal = normal;
            vertices[index + 1].normal = normal;
            vertices[index + 1 + xSizeSlider.val].normal = normal;

            index++;
        }
        index++;
    }

    int tris = 0;
    int vert = 0;
    for (int z = 0; z < zSizeSlider.val; ++z) {
        for (int x = 0; x < xSizeSlider.val; ++x) {
            // One quad
            indices[tris + 0] = vert;
            indices[tris + 1] = vert + 1;
            indices[tris + 2] = vert + xSizeSlider.val + 1;
            indices[tris + 3] = vert + 1;
            indices[tris + 4] = vert + xSizeSlider.val + 2;
            indices[tris + 5] = vert + xSizeSlider.val + 1;

            vert++;
            tris += 6;
        }
        vert++;
    }


    useStagingBuffer(vertices, vertexCount, indices, indexCount);

    delete[] vertices;
    delete[] indices;

}

void Terrain::update() {

}

std::string Terrain::getType() {
    return this->type;
}

int counter = 0;

void Terrain::onUIUpdate(UISettings uiSettings) {
    if (uiSettings.toggleGridSize) {
        counter++;
        generateSquare();
        printf("Regenerated grid %d times\n", counter);
    }

}

void Terrain::prepareObject() {
    prepareUniformBuffers(b.UBCount);
    createDescriptorSetLayout();
    createDescriptors(b.UBCount);

    VkPipelineShaderStageCreateInfo vs = loadShader("myScene/spv/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineShaderStageCreateInfo fs = loadShader("myScene/spv/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {{vs}, {fs}};
    createPipeline(*b.renderPass, shaders);
}

/*
 * void Terrain::updateUniformBufferData(Base::Render renderData) {
    UBOMatrix mat{};
    mat.model = glm::mat4(1.0f);
    mat.projection = renderData.camera->matrices.perspective;
    mat.view = renderData.camera->matrices.view;

    mat.model = glm::translate(mat.model, glm::vec3(-2.0f, 0.0f, 0.0f));

    MyModel::updateUniformBufferData(renderData.index, renderData.params, &mat);
}
 */

void Terrain::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    MyModel::draw(commandBuffer, i);
}

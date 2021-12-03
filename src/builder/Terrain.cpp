//
// Created by magnus on 9/6/21.
//

#include <glm/glm.hpp>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>
#include "Terrain.h"


void Terrain::setup(SetupVars vars) {
    printf("Terrain setup\n");
    vkMyModel::device = vars.device;

    xSizeSlider.name = "xSize";
    xSizeSlider.lowRange = 0;
    xSizeSlider.highRange = 200;
    xSizeSlider.val = 3;
    vars.ui->createIntSlider(&xSizeSlider);

    zSizeSlider.name = "zSize";
    zSizeSlider.lowRange = 0;
    zSizeSlider.highRange = 200;
    zSizeSlider.val = 3;
    vars.ui->createIntSlider(&zSizeSlider);

    generateSquare();

}

void Terrain::generateSquare() {
    // 16*16 mesh as our ground
    // Get square size from input
    int v = 0;
    auto *pn = new PerlinNoise(123);



    uint32_t vertexCount = (xSizeSlider.val + 1) * (zSizeSlider.val + 1);
    auto* vertices = new Vertex[vertexCount + 1];
    uint32_t indexCount = xSizeSlider.val * zSizeSlider.val * 6;
    auto* indices = new uint32_t[indexCount + 1];


    // Alloc memory for vertices and indices
    for (int z = 0; z <= zSizeSlider.val; ++z) {
        for (int x = 0; x <= xSizeSlider.val; ++x) {
            Vertex vertex{};

            // Use the grid size to determine the perlin noise image.
            double i = (double) x / ((double) xSizeSlider.val);
            double j = (double) z / ((double) zSizeSlider.val);
            double n = pn->noise(100 * i, 100 * j, 0.8) / 5;

            double xPos = (double) x / 10;
            double zPos = (double) z / 10;

            vertex.pos = glm::vec3(xPos, n, zPos);
            vertices[v] = vertex;
            v++;

            // calculate Normals for every 3 vertices
            if ((v % 3) == 0) {
                glm::vec3 A = vertices[v - 2].pos;
                glm::vec3 B = vertices[v - 1].pos;
                glm::vec3 C = vertices[v].pos;
                glm::vec3 AB = B - A;
                glm::vec3 AC = C - A;

                glm::vec3 normal = glm::cross(AC, AB);
                normal = glm::normalize(normal);
                // Give normal to last three vertices
                vertices[v - 2].normal = normal;
                vertices[v - 1].normal = normal;
                vertices[v].normal = normal;
            }
        }
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

vkMyModel Terrain::getSceneObject() {
    return *vkMyModel::model;
}
int counter = 0;

void Terrain::onUIUpdate(UISettings uiSettings) {
    if (uiSettings.toggleGridSize){
        counter++;
        generateSquare();
        printf("Regenerated grid %d times\n", counter);
    }

}

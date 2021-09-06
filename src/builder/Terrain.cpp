//
// Created by magnus on 9/6/21.
//

#include <glm/glm.hpp>
#include <ar_engine/external/Perlin_Noise/PerlinNoise.h>
#include "Terrain.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
};


void Terrain::setup() {
    printf("Terrain setup\n");
    generateSquare();

}

void Terrain::generateSquare() {
    // 16*16 mesh as our ground
    // Get square size from input
    int xSize = 12;
    int zSize = 12;

    int v = 0;
    auto* pn = new PerlinNoise(123);

    uint32_t vertexCount = (xSize + 1) * (zSize + 1);
    uint32_t indexCount = xSize * zSize * 6;
    // Alloc memory for vertices and indices
    std::vector<Vertex> vertices(vertexCount);
    for (int z = 0; z <= zSize; ++z) {
        for (int x = 0; x <= xSize; ++x) {
            Vertex vertex{};

            // Use the grid size to determine the perlin noise image.
            double i = (double) x / ((double) xSize);
            double j = (double) z / ((double) zSize);
            double n = pn->noise(1 * i, 1 * j, 0.8);
            vertex.pos = glm::vec3(x, n, z);
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

    std::vector<uint32_t> indices(indexCount);
    int tris = 0;
    int vert = 0;
    for (int z = 0; z < zSize; ++z) {
        for (int x = 0; x < xSize; ++x) {
            // One quad
            indices[tris + 0] = vert;
            indices[tris + 1] = vert + 1;
            indices[tris + 2] = vert + xSize + 1;
            indices[tris + 3] = vert + 1;
            indices[tris + 4] = vert + xSize + 2;
            indices[tris + 5] = vert + xSize + 1;

            vert++;
            tris += 6;
        }
        vert++;
    }
}

void Terrain::update() {

}

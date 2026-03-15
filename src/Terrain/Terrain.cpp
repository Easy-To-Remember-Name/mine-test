#include "Terrain.h"
#include "../../include/libs/SimplexNoise.h"
#include "Cube.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace Terrain {
    void generate(
        int terrainSize,
        float scale1,
        float scale2,
        float maxHeight,
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices,
        unsigned int& indexOffset,
        std::vector<std::vector<int>>& heightMap
    ) {
        // ====== Build height map ======
        heightMap.resize(terrainSize, std::vector<int>(terrainSize));

        SimplexNoise noiseGenerator;

        for (int x = 0; x < terrainSize; x++) {
            for (int z = 0; z < terrainSize; z++) {
                int worldX = x - terrainSize / 2;
                int worldZ = z - terrainSize / 2;

                float n1 = noiseGenerator.noise(worldX * scale1, worldZ * scale1);
                float n2 = noiseGenerator.noise(worldX * scale2, worldZ * scale2) * 0.4f;
                float avgNoise = glm::clamp(n1 + n2, -1.0f, 1.0f);

                int heightVal = static_cast<int>(round(((avgNoise + 1.0f) / 2.0f) * maxHeight));
                if (heightVal < 1) heightVal = 1;

                heightMap[x][z] = heightVal;
            }
        }

        // ====== Generate visible cubes only ======
        for (int x = 0; x < terrainSize; x++) {
            for (int z = 0; z < terrainSize; z++) {
                int worldX = x - terrainSize / 2;
                int worldZ = z - terrainSize / 2;
                int h = heightMap[x][z];

                // Always add the top cube
                float normalizedTop = float(h - 1) / maxHeight;
                Cube::addCube(vertices, indices, glm::vec3(worldX, h - 1, worldZ), normalizedTop, indexOffset);

                // Add underground cubes only if exposed
                for (int y = 0; y < h - 1; y++) {
                    bool exposed = false;

                    if (x == 0 || heightMap[x - 1][z] <= y) exposed = true;
                    if (x == terrainSize - 1 || heightMap[x + 1][z] <= y) exposed = true;
                    if (z == 0 || heightMap[x][z - 1] <= y) exposed = true;
                    if (z == terrainSize - 1 || heightMap[x][z + 1] <= y) exposed = true;

                    if (exposed) {
                        float normalized = float(y) / maxHeight;
                        Cube::addCube(vertices, indices, glm::vec3(worldX, y, worldZ), normalized, indexOffset);
                    }
                }
            }
        }
    }
}

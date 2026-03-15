#pragma once
#include <vector>
#include <glm/glm.hpp>

// Legacy terrain system - kept for reference
// The new chunk-based system is in World.h/World.cpp
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
    );
}
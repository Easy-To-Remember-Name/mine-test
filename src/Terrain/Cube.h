#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace Cube {
    void addCube(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos, 
                 float heightValue,
                 unsigned int& indexOffset);
}
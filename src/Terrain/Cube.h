#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace Cube {
    enum Face {
        FRONT = 0,
        BACK,
        LEFT,
        RIGHT,
        TOP,
        BOTTOM
    };

    void addFace(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos,
                 float heightValue,
                 unsigned int& indexOffset,
                 Face face);

    void addCube(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos,
                 float heightValue,
                 unsigned int& indexOffset);
}

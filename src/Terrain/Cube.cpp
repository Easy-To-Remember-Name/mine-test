#include "Cube.h"

namespace Cube {
    static void appendVertex(std::vector<float>& vertices, const glm::vec3& p, float heightValue, float u, float v) {
        vertices.push_back(p.x);
        vertices.push_back(p.y);
        vertices.push_back(p.z);
        vertices.push_back(heightValue);
        vertices.push_back(u);
        vertices.push_back(v);
    }

    void addFace(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos,
                 float heightValue,
                 unsigned int& indexOffset,
                 Face face) {
        const float minX = pos.x - 0.5f;
        const float maxX = pos.x + 0.5f;
        const float minY = pos.y - 0.5f;
        const float maxY = pos.y + 0.5f;
        const float minZ = pos.z - 0.5f;
        const float maxZ = pos.z + 0.5f;

        glm::vec3 quad[4];
        switch (face) {
            case FRONT:
                quad[0] = {minX, minY, maxZ};
                quad[1] = {maxX, minY, maxZ};
                quad[2] = {maxX, maxY, maxZ};
                quad[3] = {minX, maxY, maxZ};
                break;
            case BACK:
                quad[0] = {maxX, minY, minZ};
                quad[1] = {minX, minY, minZ};
                quad[2] = {minX, maxY, minZ};
                quad[3] = {maxX, maxY, minZ};
                break;
            case LEFT:
                quad[0] = {minX, minY, minZ};
                quad[1] = {minX, minY, maxZ};
                quad[2] = {minX, maxY, maxZ};
                quad[3] = {minX, maxY, minZ};
                break;
            case RIGHT:
                quad[0] = {maxX, minY, maxZ};
                quad[1] = {maxX, minY, minZ};
                quad[2] = {maxX, maxY, minZ};
                quad[3] = {maxX, maxY, maxZ};
                break;
            case TOP:
                quad[0] = {minX, maxY, minZ};
                quad[1] = {minX, maxY, maxZ};
                quad[2] = {maxX, maxY, maxZ};
                quad[3] = {maxX, maxY, minZ};
                break;
            case BOTTOM:
                quad[0] = {minX, minY, minZ};
                quad[1] = {maxX, minY, minZ};
                quad[2] = {maxX, minY, maxZ};
                quad[3] = {minX, minY, maxZ};
                break;
        }

        float uMin = 0.0f;
        float uMax = 1.0f;
        float vMin = 0.0f;
        float vMax = 1.0f;

        switch (face) {
            case FRONT:
                uMin = 0.0f;  uMax = 0.25f;
                vMin = 0.0f;  vMax = 0.25f;
                break;
            case BACK:
                uMin = 0.25f; uMax = 0.5f;
                vMin = 0.0f;  vMax = 0.25f;
                break;
            case LEFT:
                uMin = 0.5f;  uMax = 0.75f;
                vMin = 0.0f;  vMax = 0.25f;
                break;
            case RIGHT:
                uMin = 0.75f; uMax = 1.0f;
                vMin = 0.0f;  vMax = 0.25f;
                break;
            case TOP:
                uMin = 0.0f;  uMax = 0.25f;
                vMin = 0.25f; vMax = 0.5f;
                break;
            case BOTTOM:
                uMin = 0.25f; uMax = 0.5f;
                vMin = 0.25f; vMax = 0.5f;
                break;
        }

        appendVertex(vertices, quad[0], heightValue, uMin, vMax);
        appendVertex(vertices, quad[1], heightValue, uMax, vMax);
        appendVertex(vertices, quad[2], heightValue, uMax, vMin);
        appendVertex(vertices, quad[3], heightValue, uMin, vMin);

        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 1);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 3);
        indices.push_back(indexOffset + 0);
        indexOffset += 4;
    }

    void addCube(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos,
                 float heightValue,
                 unsigned int& indexOffset) {
        addFace(vertices, indices, pos, heightValue, indexOffset, FRONT);
        addFace(vertices, indices, pos, heightValue, indexOffset, BACK);
        addFace(vertices, indices, pos, heightValue, indexOffset, LEFT);
        addFace(vertices, indices, pos, heightValue, indexOffset, RIGHT);
        addFace(vertices, indices, pos, heightValue, indexOffset, TOP);
        addFace(vertices, indices, pos, heightValue, indexOffset, BOTTOM);
    }
}

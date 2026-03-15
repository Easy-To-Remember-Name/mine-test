#include "Cube.h"

namespace Cube {
    void addCube(std::vector<float>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3 pos, float heightValue,
                 unsigned int& indexOffset) {
        
        // Cube vertices with texture coordinates
        float cubeVerts[] = {
            // positions              // tex coords
            -0.5f, -0.5f,  0.5f,     0.0f, 0.25f,    // Front face
             0.5f, -0.5f,  0.5f,     0.25f, 0.25f,
             0.5f,  0.5f,  0.5f,     0.25f, 0.0f,
            -0.5f,  0.5f,  0.5f,     0.0f, 0.0f,
            
            -0.5f, -0.5f, -0.5f,     0.25f, 0.25f,   // Back face
             0.5f, -0.5f, -0.5f,     0.5f, 0.25f,
             0.5f,  0.5f, -0.5f,     0.5f, 0.0f,
            -0.5f,  0.5f, -0.5f,     0.25f, 0.0f,
            
            -0.5f, -0.5f, -0.5f,     0.5f, 0.25f,    // Left face
            -0.5f, -0.5f,  0.5f,     0.75f, 0.25f,
            -0.5f,  0.5f,  0.5f,     0.75f, 0.0f,
            -0.5f,  0.5f, -0.5f,     0.5f, 0.0f,
            
             0.5f, -0.5f, -0.5f,     0.75f, 0.25f,   // Right face
             0.5f, -0.5f,  0.5f,     1.0f, 0.25f,
             0.5f,  0.5f,  0.5f,     1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,     0.75f, 0.0f,
            
            -0.5f,  0.5f, -0.5f,     0.0f, 0.25f,    // Top face
             0.5f,  0.5f, -0.5f,     0.25f, 0.25f,
             0.5f,  0.5f,  0.5f,     0.25f, 0.5f,
            -0.5f,  0.5f,  0.5f,     0.0f, 0.5f,
            
            -0.5f, -0.5f, -0.5f,     0.25f, 0.25f,   // Bottom face
             0.5f, -0.5f, -0.5f,     0.5f, 0.25f,
             0.5f, -0.5f,  0.5f,     0.5f, 0.5f,
            -0.5f, -0.5f,  0.5f,     0.25f, 0.5f
        };
        
        // Cube indices (2 triangles per face)
        unsigned int cubeInd[] = {
            0,  1,  2,   2,  3,  0,    // Front face
            4,  5,  6,   6,  7,  4,    // Back face
            8,  9,  10,  10, 11, 8,    // Left face
            12, 13, 14,  14, 15, 12,   // Right face
            16, 17, 18,  18, 19, 16,   // Top face
            20, 21, 22,  22, 23, 20    // Bottom face
        };

        // Add vertices with position offset and height value
        for(int i = 0; i < 24; i++) {
            vertices.push_back(cubeVerts[i * 5] + pos.x);      // x position
            vertices.push_back(cubeVerts[i * 5 + 1] + pos.y);  // y position  
            vertices.push_back(cubeVerts[i * 5 + 2] + pos.z);  // z position
            vertices.push_back(heightValue);                    // height value for shader
            vertices.push_back(cubeVerts[i * 5 + 3]);          // u texture coordinate
            vertices.push_back(cubeVerts[i * 5 + 4]);          // v texture coordinate
        }

        // Add indices with offset
        for(int i = 0; i < 36; i++) {
            indices.push_back(cubeInd[i] + indexOffset);
        }
        
        indexOffset += 24; // 24 vertices per cube
    }
}
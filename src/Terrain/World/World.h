#pragma once
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../include/libs/SimplexNoise.h"

struct Chunk {
    int chunkX, chunkZ;
    std::vector<int> heightMap;
    std::vector<bool> caveMap; // 3D cave data flattened
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    bool generated = false;
};

enum BiomeType {
    PLAINS = 0,
    MOUNTAINS = 1,
    HILLS = 2,
    DESERT = 3
};

enum TextureType {
    GRASS_TEX = 0,    // Plains grass
    STONE_TEX = 1,    // Mountains/caves
    DIRT_TEX = 2,     // Underground
    SNOW_TEX = 3      // Mountain peaks
};

class World {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int RENDER_DISTANCE = 32;
    static constexpr float MAX_HEIGHT = 64.0f;
    static constexpr int MIN_HEIGHT = 5;
    
    // Noise scales for different features
    static constexpr float TERRAIN_SCALE = 0.008f;     // Base terrain
    static constexpr float MOUNTAIN_SCALE = 0.003f;    // Large mountain regions
    static constexpr float DETAIL_SCALE = 0.05f;       // Small details
    static constexpr float CAVE_SCALE = 0.1f;          // Cave systems
    static constexpr float BIOME_SCALE = 0.002f;       // Biome regions

    World();
    ~World();

    void updateChunks(const glm::vec3& playerPos);
    void render();
    int getHeightAtWorld(int worldX, int worldZ) const;
    bool isCaveAtWorld(int worldX, int worldY, int worldZ) const;
    std::vector<glm::vec3> getCollisionBlocks(const glm::vec3& playerMin, const glm::vec3& playerMax) const;

private:
    std::unordered_map<long long, Chunk*> chunks;
    SimplexNoise terrainNoise;
    SimplexNoise mountainNoise;
    SimplexNoise caveNoise;
    SimplexNoise biomeNoise;
    SimplexNoise detailNoise;

    void generateChunk(Chunk* chunk);
    void unloadChunk(Chunk* chunk);
    bool isExposed(int lx, int lz, int y, Chunk* chunk) const;
    
    // New biome and cave generation functions
    BiomeType getBiome(int worldX, int worldZ) const;
    int generateHeight(int worldX, int worldZ, BiomeType biome) const;
    bool generateCave(int worldX, int worldY, int worldZ) const;
    float getTextureValue(int worldX, int worldZ, int y, int surfaceHeight, bool isTop, BiomeType biome) const;
    float calculateSlope(int worldX, int worldZ) const;

    // Helper functions
    inline int localIndex(int x, int z) const { return x * CHUNK_SIZE + z; }
    inline int caveIndex(int x, int y, int z) const { return x * CHUNK_SIZE * (int)MAX_HEIGHT + y * CHUNK_SIZE + z; }
    inline long long chunkKey(int x, int z) const { return ((long long)x << 32) | (z & 0xFFFFFFFF); }
};
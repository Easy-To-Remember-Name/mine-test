#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <glm/glm.hpp>
#include "../include/libs/SimplexNoise.h"

struct Chunk {
    int chunkX, chunkZ;
    std::vector<int> heightMap;
    std::unordered_set<int> caveVoxels;
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

class World {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int RENDER_DISTANCE = 10;
    static constexpr int LOAD_DISTANCE = 12;
    static constexpr int CHUNK_GEN_BUDGET_PER_FRAME = 2;
    static constexpr float MAX_HEIGHT = 64.0f;
    static constexpr int MIN_HEIGHT = 5;

    static constexpr float TERRAIN_SCALE = 0.008f;
    static constexpr float MOUNTAIN_SCALE = 0.003f;
    static constexpr float DETAIL_SCALE = 0.05f;
    static constexpr float CAVE_SCALE = 0.1f;
    static constexpr float BIOME_SCALE = 0.002f;

    World();
    ~World();

    void updateChunks(const glm::vec3& playerPos);
    void render();
    int getHeightAtWorld(int worldX, int worldZ) const;
    bool isCaveAtWorld(int worldX, int worldY, int worldZ) const;
    std::vector<glm::vec3> getCollisionBlocks(const glm::vec3& playerMin, const glm::vec3& playerMax) const;

private:
    std::unordered_map<long long, Chunk*> chunks;
    std::deque<glm::ivec2> chunkGenerationQueue;
    std::unordered_set<long long> queuedChunkKeys;

    SimplexNoise terrainNoise;
    SimplexNoise mountainNoise;
    SimplexNoise caveNoise;
    SimplexNoise biomeNoise;
    SimplexNoise detailNoise;

    void generateChunk(Chunk* chunk);
    void unloadChunk(Chunk* chunk);

    BiomeType getBiome(int worldX, int worldZ) const;
    int generateHeight(int worldX, int worldZ, BiomeType biome) const;
    bool generateCave(int worldX, int worldY, int worldZ) const;
    float getTextureValue(int worldX, int worldZ, int y, int surfaceHeight, bool isTop, BiomeType biome) const;

    inline int localIndex(int x, int z) const { return x * CHUNK_SIZE + z; }
    inline int caveVoxelKey(int x, int y, int z) const { return x + CHUNK_SIZE * (z + CHUNK_SIZE * y); }
    inline long long chunkKey(int x, int z) const { return ((long long)x << 32) | (z & 0xFFFFFFFF); }
};

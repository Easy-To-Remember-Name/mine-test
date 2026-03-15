#include "World.h"
#include "../Cube.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <chrono>

World::World() {
    terrainNoise = SimplexNoise();
    mountainNoise = SimplexNoise();
    caveNoise = SimplexNoise();
    biomeNoise = SimplexNoise();
    detailNoise = SimplexNoise();
}

World::~World() {
    for (auto &p : chunks) {
        unloadChunk(p.second);
    }
    chunks.clear();
}

BiomeType World::getBiome(int worldX, int worldZ) const {
    float biomeValue = biomeNoise.noise(worldX * BIOME_SCALE, worldZ * BIOME_SCALE);
    float mountainValue = mountainNoise.noise(worldX * MOUNTAIN_SCALE, worldZ * MOUNTAIN_SCALE);

    if (mountainValue > 0.4f && biomeValue > 0.2f) {
        return MOUNTAINS;
    }
    if (mountainValue > 0.1f || biomeValue > 0.3f) {
        return HILLS;
    }
    if (biomeValue < -0.3f) {
        return DESERT;
    }
    return PLAINS;
}

int World::generateHeight(int worldX, int worldZ, BiomeType biome) const {
    float baseNoise = terrainNoise.noise(worldX * TERRAIN_SCALE, worldZ * TERRAIN_SCALE);
    float mountainValue = mountainNoise.noise(worldX * MOUNTAIN_SCALE, worldZ * MOUNTAIN_SCALE);
    float detailValue = detailNoise.noise(worldX * DETAIL_SCALE, worldZ * DETAIL_SCALE) * 0.05f;

    int height = MIN_HEIGHT;

    switch (biome) {
        case PLAINS:
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.5f + 0.5f) * 6.0f);
            height += static_cast<int>(detailValue * 1.5f);
            break;
        case HILLS:
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.5f + 0.5f) * 12.0f);
            height += static_cast<int>((mountainValue * 0.3f + 0.3f) * 6.0f);
            height += static_cast<int>(detailValue * 2.0f);
            break;
        case MOUNTAINS: {
            float mountainHeight = (mountainValue * 0.4f + 0.4f) * 25.0f;
            float terrainHeight = (baseNoise * 0.3f + 0.3f) * 12.0f;
            height = MIN_HEIGHT + static_cast<int>(mountainHeight + terrainHeight);
            height += static_cast<int>(detailValue * 3.0f);
            break;
        }
        case DESERT:
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.3f + 0.3f) * 4.0f);
            height += static_cast<int>(detailValue * 2.0f);
            break;
    }

    return glm::clamp(height, MIN_HEIGHT, static_cast<int>(MAX_HEIGHT));
}

bool World::generateCave(int worldX, int worldY, int worldZ) const {
    if (worldY < 2 || worldY > MAX_HEIGHT * 0.6f) return false;

    float caveValue = caveNoise.noise(worldX * CAVE_SCALE, worldY * CAVE_SCALE, worldZ * CAVE_SCALE);
    float cavernNoise = caveNoise.noise(worldX * CAVE_SCALE * 0.5f, worldY * CAVE_SCALE * 0.5f, worldZ * CAVE_SCALE * 0.5f);

    float threshold = 0.55f + (worldY / MAX_HEIGHT) * 0.1f;
    return caveValue > threshold || cavernNoise > 0.65f;
}

float World::getTextureValue(int worldX, int worldZ, int y, int surfaceHeight, bool isTop, BiomeType biome) const {
    float depthFromSurface = surfaceHeight - y;
    float height = static_cast<float>(y) / MAX_HEIGHT;

    if (isTop) {
        float surfaceRatio = static_cast<float>(surfaceHeight) / MAX_HEIGHT;
        switch (biome) {
            case PLAINS:
                return 0.25f + (surfaceRatio * 0.1f);
            case HILLS:
                return 0.35f + (surfaceRatio * 0.2f);
            case MOUNTAINS:
                return surfaceRatio > 0.7f ? 0.85f : (surfaceRatio > 0.5f ? 0.6f : 0.35f);
            case DESERT:
                return 0.75f;
        }
    } else {
        if (depthFromSurface <= 1) {
            if (biome == MOUNTAINS) return 0.5f;
            if (biome == DESERT) return 0.75f;
            return 0.3f;
        }
        if (depthFromSurface <= 4) {
            return 0.5f + (biome == DESERT ? 0.15f : 0.0f);
        }
        return 0.6f;
    }

    return height;
}

void World::generateChunk(Chunk* chunk) {
    constexpr int PADDED = CHUNK_SIZE + 2;
    std::vector<int> cachedHeights(PADDED * PADDED, 0);
    std::vector<BiomeType> cachedBiomes(PADDED * PADDED, PLAINS);
    auto cacheIndex = [](int x, int z) { return x * PADDED + z; };

    const int startX = chunk->chunkX * CHUNK_SIZE;
    const int startZ = chunk->chunkZ * CHUNK_SIZE;

    chunk->heightMap.assign(CHUNK_SIZE * CHUNK_SIZE, 0);
    chunk->caveVoxels.clear();

    for (int cx = -1; cx <= CHUNK_SIZE; ++cx) {
        for (int cz = -1; cz <= CHUNK_SIZE; ++cz) {
            const int worldX = startX + cx;
            const int worldZ = startZ + cz;
            const BiomeType biome = getBiome(worldX, worldZ);
            cachedBiomes[cacheIndex(cx + 1, cz + 1)] = biome;
            cachedHeights[cacheIndex(cx + 1, cz + 1)] = generateHeight(worldX, worldZ, biome);
        }
    }

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int h = cachedHeights[cacheIndex(lx + 1, lz + 1)];
            chunk->heightMap[localIndex(lx, lz)] = h;

            int worldX = startX + lx;
            int worldZ = startZ + lz;
            for (int y = 0; y < h && y < MAX_HEIGHT; y++) {
                if (generateCave(worldX, y, worldZ)) {
                    chunk->caveVoxels.insert(caveVoxelKey(lx, y, lz));
                }
            }
        }
    }

    chunk->vertices.clear();
    chunk->indices.clear();
    unsigned int indexOffset = 0;

    const int neighbors[6][3] = { {0,0,1}, {0,0,-1}, {-1,0,0}, {1,0,0}, {0,1,0}, {0,-1,0} };
    const Cube::Face faces[6] = { Cube::FRONT, Cube::BACK, Cube::LEFT, Cube::RIGHT, Cube::TOP, Cube::BOTTOM };

    auto isSolidAt = [&](int nx, int ny, int nz) {
        if (ny < 0 || ny >= MAX_HEIGHT) return false;

        if (nx >= 0 && nx < CHUNK_SIZE && nz >= 0 && nz < CHUNK_SIZE) {
            int h = chunk->heightMap[localIndex(nx, nz)];
            if (ny >= h) return false;
            return chunk->caveVoxels.find(caveVoxelKey(nx, ny, nz)) == chunk->caveVoxels.end();
        }

        int worldX = startX + nx;
        int worldZ = startZ + nz;

        int edgeX = nx + 1;
        int edgeZ = nz + 1;
        int neighborHeight;
        if (edgeX >= 0 && edgeX < PADDED && edgeZ >= 0 && edgeZ < PADDED) {
            neighborHeight = cachedHeights[cacheIndex(edgeX, edgeZ)];
        } else {
            neighborHeight = getHeightAtWorld(worldX, worldZ);
        }

        if (ny >= neighborHeight) return false;
        return !generateCave(worldX, ny, worldZ);
    };

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            const int worldX = startX + lx;
            const int worldZ = startZ + lz;
            const int h = chunk->heightMap[localIndex(lx, lz)];
            const BiomeType biome = cachedBiomes[cacheIndex(lx + 1, lz + 1)];

            for (int y = 0; y < h && y < MAX_HEIGHT; y++) {
                if (chunk->caveVoxels.find(caveVoxelKey(lx, y, lz)) != chunk->caveVoxels.end()) {
                    continue;
                }

                bool emitted = false;
                float textureValue = getTextureValue(worldX, worldZ, y, h, y == h - 1, biome);
                for (int i = 0; i < 6; ++i) {
                    const int nx = lx + neighbors[i][0];
                    const int ny = y + neighbors[i][1];
                    const int nz = lz + neighbors[i][2];
                    if (!isSolidAt(nx, ny, nz)) {
                        Cube::addFace(chunk->vertices, chunk->indices, glm::vec3(worldX, y, worldZ), textureValue, indexOffset, faces[i]);
                        emitted = true;
                    }
                }
                (void)emitted;
            }
        }
    }

    glGenVertexArrays(1, &chunk->VAO);
    glGenBuffers(1, &chunk->VBO);
    glGenBuffers(1, &chunk->EBO);

    glBindVertexArray(chunk->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
    glBufferData(GL_ARRAY_BUFFER, chunk->vertices.size() * sizeof(float), chunk->vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunk->indices.size() * sizeof(unsigned int), chunk->indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));

    glBindVertexArray(0);
    chunk->generated = true;
}

void World::unloadChunk(Chunk* chunk) {
    if (!chunk) return;
    if (chunk->generated) {
        glDeleteVertexArrays(1, &chunk->VAO);
        glDeleteBuffers(1, &chunk->VBO);
        glDeleteBuffers(1, &chunk->EBO);
    }
    delete chunk;
}

void World::updateChunks(const glm::vec3& playerPos) {
    const auto updateStart = std::chrono::high_resolution_clock::now();

    int playerChunkX = static_cast<int>(floor(playerPos.x / CHUNK_SIZE));
    int playerChunkZ = static_cast<int>(floor(playerPos.z / CHUNK_SIZE));

    std::vector<glm::ivec2> desired;
    desired.reserve((LOAD_DISTANCE * 2 + 1) * (LOAD_DISTANCE * 2 + 1));

    for (int dx = -LOAD_DISTANCE; dx <= LOAD_DISTANCE; dx++) {
        for (int dz = -LOAD_DISTANCE; dz <= LOAD_DISTANCE; dz++) {
            int cx = playerChunkX + dx;
            int cz = playerChunkZ + dz;
            long long key = chunkKey(cx, cz);
            if (chunks.find(key) == chunks.end() && queuedChunkKeys.find(key) == queuedChunkKeys.end()) {
                desired.push_back({cx, cz});
            }
        }
    }

    std::sort(desired.begin(), desired.end(), [playerChunkX, playerChunkZ](const glm::ivec2& a, const glm::ivec2& b) {
        int adx = a.x - playerChunkX;
        int adz = a.y - playerChunkZ;
        int bdx = b.x - playerChunkX;
        int bdz = b.y - playerChunkZ;
        return (adx * adx + adz * adz) < (bdx * bdx + bdz * bdz);
    });

    for (const auto& c : desired) {
        long long key = chunkKey(c.x, c.y);
        chunkGenerationQueue.push_back(c);
        queuedChunkKeys.insert(key);
    }

    int generatedThisFrame = 0;
    while (!chunkGenerationQueue.empty() && generatedThisFrame < CHUNK_GEN_BUDGET_PER_FRAME) {
        glm::ivec2 next = chunkGenerationQueue.front();
        chunkGenerationQueue.pop_front();
        long long key = chunkKey(next.x, next.y);
        queuedChunkKeys.erase(key);

        if (chunks.find(key) != chunks.end()) {
            continue;
        }

        Chunk* c = new Chunk();
        c->chunkX = next.x;
        c->chunkZ = next.y;
        generateChunk(c);
        chunks[key] = c;
        generatedThisFrame++;
    }

    std::vector<long long> toRemove;
    for (auto &p : chunks) {
        Chunk* c = p.second;
        int dx = c->chunkX - playerChunkX;
        int dz = c->chunkZ - playerChunkZ;
        if (abs(dx) > LOAD_DISTANCE || abs(dz) > LOAD_DISTANCE) {
            unloadChunk(c);
            toRemove.push_back(p.first);
        }
    }
    for (auto k : toRemove) {
        chunks.erase(k);
    }

    chunkGenerationQueue.erase(
        std::remove_if(chunkGenerationQueue.begin(), chunkGenerationQueue.end(),
                       [playerChunkX, playerChunkZ](const glm::ivec2& c) {
                           return std::abs(c.x - playerChunkX) > LOAD_DISTANCE || std::abs(c.y - playerChunkZ) > LOAD_DISTANCE;
                       }),
        chunkGenerationQueue.end());

    const auto updateEnd = std::chrono::high_resolution_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(updateEnd - updateStart).count();
    if (elapsedMs > 8) {
        std::cout << "[World] updateChunks took " << elapsedMs << "ms | generated=" << generatedThisFrame
                  << " | queued=" << chunkGenerationQueue.size() << " | loaded=" << chunks.size() << '\n';
    }
}

void World::render() {
    for (auto &p : chunks) {
        Chunk* c = p.second;
        if (!c->generated) continue;

        glBindVertexArray(c->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(c->indices.size()), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

int World::getHeightAtWorld(int worldX, int worldZ) const {
    int chunkX = static_cast<int>(floor(static_cast<float>(worldX) / CHUNK_SIZE));
    int chunkZ = static_cast<int>(floor(static_cast<float>(worldZ) / CHUNK_SIZE));
    long long key = chunkKey(chunkX, chunkZ);

    auto it = chunks.find(key);
    if (it == chunks.end()) {
        BiomeType biome = getBiome(worldX, worldZ);
        return generateHeight(worldX, worldZ, biome);
    }

    const Chunk* c = it->second;
    int lx = worldX - chunkX * CHUNK_SIZE;
    int lz = worldZ - chunkZ * CHUNK_SIZE;

    if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE) return MIN_HEIGHT;
    return c->heightMap[localIndex(lx, lz)];
}

bool World::isCaveAtWorld(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= MAX_HEIGHT) return false;

    int chunkX = static_cast<int>(floor(static_cast<float>(worldX) / CHUNK_SIZE));
    int chunkZ = static_cast<int>(floor(static_cast<float>(worldZ) / CHUNK_SIZE));
    long long key = chunkKey(chunkX, chunkZ);

    auto it = chunks.find(key);
    if (it == chunks.end()) {
        return generateCave(worldX, worldY, worldZ);
    }

    int lx = worldX - chunkX * CHUNK_SIZE;
    int lz = worldZ - chunkZ * CHUNK_SIZE;

    if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE) return false;
    return it->second->caveVoxels.find(caveVoxelKey(lx, worldY, lz)) != it->second->caveVoxels.end();
}

std::vector<glm::vec3> World::getCollisionBlocks(const glm::vec3& playerMin, const glm::vec3& playerMax) const {
    std::vector<glm::vec3> blocks;

    int minChunkX = static_cast<int>(floor(playerMin.x / CHUNK_SIZE));
    int maxChunkX = static_cast<int>(floor(playerMax.x / CHUNK_SIZE));
    int minChunkZ = static_cast<int>(floor(playerMin.z / CHUNK_SIZE));
    int maxChunkZ = static_cast<int>(floor(playerMax.z / CHUNK_SIZE));

    for (int cx = minChunkX; cx <= maxChunkX; cx++) {
        for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
            long long key = chunkKey(cx, cz);
            auto it = chunks.find(key);
            if (it == chunks.end()) continue;

            const Chunk* chunk = it->second;
            if (!chunk->generated) continue;

            int startX = cx * CHUNK_SIZE;
            int startZ = cz * CHUNK_SIZE;

            int localMinX = std::max(0, static_cast<int>(floor(playerMin.x)) - startX);
            int localMaxX = std::min(CHUNK_SIZE - 1, static_cast<int>(floor(playerMax.x)) - startX);
            int localMinZ = std::max(0, static_cast<int>(floor(playerMin.z)) - startZ);
            int localMaxZ = std::min(CHUNK_SIZE - 1, static_cast<int>(floor(playerMax.z)) - startZ);
            int minY = std::max(0, static_cast<int>(floor(playerMin.y)));
            int maxY = std::min(static_cast<int>(MAX_HEIGHT - 1), static_cast<int>(ceil(playerMax.y)));

            for (int lx = localMinX; lx <= localMaxX; lx++) {
                for (int lz = localMinZ; lz <= localMaxZ; lz++) {
                    int h = chunk->heightMap[localIndex(lx, lz)];
                    int worldX = startX + lx;
                    int worldZ = startZ + lz;

                    for (int y = minY; y <= maxY && y < h; y++) {
                        if (chunk->caveVoxels.find(caveVoxelKey(lx, y, lz)) != chunk->caveVoxels.end()) continue;
                        blocks.push_back(glm::vec3(worldX, y, worldZ));
                    }
                }
            }
        }
    }

    return blocks;
}

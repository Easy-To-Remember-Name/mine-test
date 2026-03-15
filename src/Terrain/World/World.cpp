#include "World.h"
#include "../Cube.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include <algorithm>

World::World() {
    // Initialize different noise generators with different seeds for variety
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
    
    // Create smoother biome transitions
    if (mountainValue > 0.4f && biomeValue > 0.2f) {
        return MOUNTAINS;
    }
    else if (mountainValue > 0.1f || biomeValue > 0.3f) {
        return HILLS;
    }
    else if (biomeValue < -0.3f) {
        return DESERT;
    }
    else {
        return PLAINS;
    }
}

int World::generateHeight(int worldX, int worldZ, BiomeType biome) const {
    float baseNoise = terrainNoise.noise(worldX * TERRAIN_SCALE, worldZ * TERRAIN_SCALE);
    float mountainValue = mountainNoise.noise(worldX * MOUNTAIN_SCALE, worldZ * MOUNTAIN_SCALE);
    float detailValue = detailNoise.noise(worldX * DETAIL_SCALE, worldZ * DETAIL_SCALE) * 0.05f; // Reduced detail
    
    int height = MIN_HEIGHT;
    
    switch (biome) {
        case PLAINS: {
            // Gentle rolling plains - much smoother
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.5f + 0.5f) * 6.0f); // Reduced from 8
            height += static_cast<int>(detailValue * 1.5f); // Reduced from 2
            break;
        }
        case HILLS: {
            // More varied terrain but smoother
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.5f + 0.5f) * 12.0f); // Reduced from 16
            height += static_cast<int>((mountainValue * 0.3f + 0.3f) * 6.0f); // Smoother mountains
            height += static_cast<int>(detailValue * 2.0f);
            break;
        }
        case MOUNTAINS: {
            // Tall peaks but smoother transitions
            float mountainHeight = (mountainValue * 0.4f + 0.4f) * 25.0f; // Reduced intensity
            float terrainHeight = (baseNoise * 0.3f + 0.3f) * 12.0f; // Smoother base
            height = MIN_HEIGHT + static_cast<int>(mountainHeight + terrainHeight);
            height += static_cast<int>(detailValue * 3.0f);
            break;
        }
        case DESERT: {
            // Very flat desert
            height = MIN_HEIGHT + static_cast<int>((baseNoise * 0.3f + 0.3f) * 4.0f); // Much flatter
            height += static_cast<int>(detailValue * 2.0f); // Small dunes
            break;
        }
    }
    
    return glm::clamp(height, MIN_HEIGHT, static_cast<int>(MAX_HEIGHT));
}

bool World::generateCave(int worldX, int worldY, int worldZ) const {
    // Don't generate caves too close to surface or too deep
    if (worldY < 2 || worldY > MAX_HEIGHT * 0.6f) return false;
    
    // Reduced cave generation for smoother terrain
    float cave1 = caveNoise.noise(worldX * CAVE_SCALE * 0.7f, worldY * CAVE_SCALE * 1.5f, worldZ * CAVE_SCALE * 0.7f);
    float cave2 = caveNoise.noise(worldX * CAVE_SCALE * 1.3f, worldY * CAVE_SCALE * 1.0f, worldZ * CAVE_SCALE * 1.3f);
    
    // Make caves less common and smoother
    bool isCave = (cave1 > -0.15f && cave1 < 0.15f) || (cave2 > -0.1f && cave2 < 0.1f);
    
    // Make caves much less likely
    float depthFactor = 1.0f - abs(worldY - MAX_HEIGHT * 0.25f) / (MAX_HEIGHT * 0.25f);
    isCave = isCave && (terrainNoise.noise(worldX * 0.015f, worldZ * 0.015f) * depthFactor > 0.2f);
    
    return isCave;
}

float World::getTextureValue(int worldX, int worldZ, int y, int surfaceHeight, bool isTop, BiomeType biome) const {
    float height = float(y) / MAX_HEIGHT;
    float surfaceRatio = float(surfaceHeight) / MAX_HEIGHT;
    float depthFromSurface = surfaceHeight - y;
    
    if (isTop) {
        // Surface block texturing based on biome
        switch (biome) {
            case PLAINS: {
                return 0.1f; // Grass texture range
            }
            case HILLS: {
                // Mix grass and stone based on slope
                float slope = calculateSlope(worldX, worldZ);
                if (slope > 0.4f) {
                    return 0.6f; // Stone on steep slopes
                } else {
                    return 0.1f + slope * 0.3f; // Grass to stone transition
                }
                break;
            }
            case MOUNTAINS: {
                if (surfaceRatio > 0.7f) {
                    return 0.85f; // Snow on peaks
                } else if (surfaceRatio > 0.5f) {
                    return 0.6f; // Stone on high areas
                } else {
                    return 0.35f; // Dirt/grass in valleys
                }
                break;
            }
            case DESERT: {
                return 0.75f; // Sandy texture
            }
        }
    } else {
        // Underground texturing
        if (depthFromSurface <= 1) {
            // Just below surface
            switch (biome) {
                case PLAINS:
                case HILLS: {
                    return 0.3f; // Dirt
                }
                case MOUNTAINS: {
                    return 0.5f; // Rocky dirt
                }
                case DESERT: {
                    return 0.75f; // Sand
                }
            }
        } else if (depthFromSurface <= 4) {
            // Shallow underground
            return 0.5f + (biome == DESERT ? 0.15f : 0.0f); // Stone
        } else {
            // Deep underground - always stone
            return 0.6f;
        }
    }
    
    return height; // Fallback
}

float World::calculateSlope(int worldX, int worldZ) const {
    int center = getHeightAtWorld(worldX, worldZ);
    int north = getHeightAtWorld(worldX, worldZ + 1);
    int south = getHeightAtWorld(worldX, worldZ - 1);
    int east = getHeightAtWorld(worldX + 1, worldZ);
    int west = getHeightAtWorld(worldX - 1, worldZ);
    
    float avgDiff = (abs(center - north) + abs(center - south) + 
                     abs(center - east) + abs(center - west)) / 4.0f;
    
    return glm::clamp(avgDiff / 4.0f, 0.0f, 1.0f); // Reduced divisor for more sensitivity
}

void World::generateChunk(Chunk* chunk) {
    // Generate heightmap and cave data
    chunk->heightMap.assign(CHUNK_SIZE * CHUNK_SIZE, 0);
    chunk->caveMap.assign(CHUNK_SIZE * CHUNK_SIZE * (int)MAX_HEIGHT, false);
    
    int startX = chunk->chunkX * CHUNK_SIZE;
    int startZ = chunk->chunkZ * CHUNK_SIZE;

    // Generate terrain
    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int worldX = startX + lx;
            int worldZ = startZ + lz;
            
            BiomeType biome = getBiome(worldX, worldZ);
            int h = generateHeight(worldX, worldZ, biome);
            chunk->heightMap[localIndex(lx, lz)] = h;
            
            // Generate caves for this column
            for (int y = 0; y < h && y < MAX_HEIGHT; y++) {
                bool isCave = generateCave(worldX, y, worldZ);
                chunk->caveMap[caveIndex(lx, y, lz)] = isCave;
            }
        }
    }

    // Build mesh
    chunk->vertices.clear();
    chunk->indices.clear();
    unsigned int indexOffset = 0;

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int worldX = startX + lx;
            int worldZ = startZ + lz;
            int h = chunk->heightMap[localIndex(lx, lz)];
            BiomeType biome = getBiome(worldX, worldZ);

            // Generate all blocks in this column
            for (int y = 0; y < h && y < MAX_HEIGHT; y++) {
                bool isCave = (y < MAX_HEIGHT) ? chunk->caveMap[caveIndex(lx, y, lz)] : false;
                
                // Skip cave blocks
                if (isCave) continue;
                
                bool isTop = (y == h - 1);
                bool shouldRender = isTop || isExposed(lx, lz, y, chunk);
                
                if (shouldRender) {
                    float textureValue = getTextureValue(worldX, worldZ, y, h, isTop, biome);
                    Cube::addCube(chunk->vertices, chunk->indices,
                                  glm::vec3(worldX, y, worldZ), textureValue, indexOffset);
                }
            }
        }
    }

    // Upload to GPU
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

bool World::isExposed(int lx, int lz, int y, Chunk* chunk) const {
    // Check if block is exposed (has air or cave adjacent)
    int neighbors[6][3] = {{-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}};
    
    for (int i = 0; i < 6; i++) {
        int nx = lx + neighbors[i][0];
        int ny = y + neighbors[i][1];  
        int nz = lz + neighbors[i][2];
        
        bool isAir = false;
        
        if (ny < 0 || ny >= MAX_HEIGHT) {
            isAir = true; // Outside height bounds
        } else if (nx >= 0 && nx < CHUNK_SIZE && nz >= 0 && nz < CHUNK_SIZE) {
            // Same chunk
            int neighborHeight = chunk->heightMap[localIndex(nx, nz)];
            if (ny >= neighborHeight) {
                isAir = true; // Above terrain
            } else if (ny < MAX_HEIGHT) {
                isAir = chunk->caveMap[caveIndex(nx, ny, nz)]; // Cave check
            }
        } else {
            // Different chunk
            int worldX = chunk->chunkX * CHUNK_SIZE + nx;
            int worldZ = chunk->chunkZ * CHUNK_SIZE + nz;
            int neighborHeight = getHeightAtWorld(worldX, worldZ);
            
            if (ny >= neighborHeight) {
                isAir = true;
            } else {
                isAir = isCaveAtWorld(worldX, ny, worldZ);
            }
        }
        
        if (isAir) return true;
    }
    
    return false;
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
    int playerChunkX = static_cast<int>(floor(playerPos.x / CHUNK_SIZE));
    int playerChunkZ = static_cast<int>(floor(playerPos.z / CHUNK_SIZE));

    // Load chunks around player
    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++) {
            int cx = playerChunkX + dx;
            int cz = playerChunkZ + dz;
            long long key = chunkKey(cx, cz);
            
            if (chunks.find(key) == chunks.end()) {
                Chunk* c = new Chunk();
                c->chunkX = cx;
                c->chunkZ = cz;
                generateChunk(c);
                chunks[key] = c;
            }
        }
    }

    // Unload distant chunks
    std::vector<long long> toRemove;
    for (auto &p : chunks) {
        Chunk* c = p.second;
        int dx = c->chunkX - playerChunkX;
        int dz = c->chunkZ - playerChunkZ;
        if (abs(dx) > RENDER_DISTANCE || abs(dz) > RENDER_DISTANCE) {
            unloadChunk(c);
            toRemove.push_back(p.first);
        }
    }
    for (auto k : toRemove) {
        chunks.erase(k);
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
        // Generate height on-the-fly for unloaded chunks
        BiomeType biome = getBiome(worldX, worldZ);
        return generateHeight(worldX, worldZ, biome);
    }
    
    const Chunk* c = it->second;
    if (!c->generated) return MIN_HEIGHT;
    
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
        // Generate cave data on-the-fly for unloaded chunks
        return generateCave(worldX, worldY, worldZ);
    }
    
    const Chunk* c = it->second;
    if (!c->generated) return false;
    
    int lx = worldX - chunkX * CHUNK_SIZE;
    int lz = worldZ - chunkZ * CHUNK_SIZE;
    
    if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE) return false;
    
    return c->caveMap[caveIndex(lx, worldY, lz)];
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
                        // Skip if it's a cave
                        if (y < MAX_HEIGHT && chunk->caveMap[caveIndex(lx, y, lz)]) continue;
                        
                        glm::vec3 blockPos(worldX, y, worldZ);
                        blocks.push_back(blockPos);
                    }
                }
            }
        }
    }
    
    return blocks;
}
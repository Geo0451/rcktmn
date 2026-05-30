#include "tilemap.h"
#include <cmath>
#include <random>
#include <algorithm>

// Helper: Clamp function
template<typename T>
T Clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(value, max_val));
}

TileMap::TileMap()
{
    grid.resize((size_t)WORLD_ROWS * (size_t)WORLD_COLS);
    for (size_t i = 0; i < grid.size(); ++i) grid[i] = {false, WHITE};
}

bool TileMap::inBounds(int col, int row) const
{
    return col >= 0 && col < WORLD_COLS && row >= 0 && row < WORLD_ROWS;
}

bool TileMap::isSolid(int col, int row) const
{
    if (!inBounds(col, row)) return false;
    return grid[(size_t)row * WORLD_COLS + (size_t)col].solid;
}

Tile& TileMap::tileAt(int col, int row)
{
    return grid[(size_t)row * WORLD_COLS + (size_t)col];
}

const Tile& TileMap::tileAt(int col, int row) const
{
    return grid[(size_t)row * WORLD_COLS + (size_t)col];
}

Rectangle TileMap::getTileRect(int col, int row) const
{
    return {
        (float)(col * TILE_SIZE),
        (float)(row * TILE_SIZE),
        (float)TILE_SIZE,
        (float)TILE_SIZE
    };
}

int TileMap::getSurfaceRow(int col) const
{
    if (col < 0 || col >= WORLD_COLS) return WORLD_ROWS;
    for (int r = 0; r < WORLD_ROWS; ++r)
        if (isSolid(col, r)) return r;
    return WORLD_ROWS;
}

void TileMap::fillPixelRect(int px, int py, int pw, int ph, bool solid, Color color)
{
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;

    int c0 =  px              / TILE_SIZE;
    int r0 =  py              / TILE_SIZE;
    int c1 = (px + pw - 1)   / TILE_SIZE;
    int r1 = (py + ph - 1)   / TILE_SIZE;

    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++)
            if (inBounds(c, r))
            {
                tileAt(c, r).solid = solid;
                tileAt(c, r).color = color;
            }
}

void TileMap::eraseAtPixel(int px, int py)
{
    int c = px / TILE_SIZE;
    int r = py / TILE_SIZE;
    if (inBounds(c, r)) tileAt(c, r) = {false, WHITE};
}

void TileMap::clear()
{
    for (size_t i = 0; i < grid.size(); ++i)
        grid[i] = {false, WHITE};
}

void TileMap::generate(unsigned int seed, float surfaceHeightFactor, float roughness,
                       float caveDensity, float caveThreshold,
                       float caveSurfaceBias, int caveSmoothIterations)
{
    std::mt19937 rng(seed);
    
    auto perlin = [&](float x, float y) -> float
    {
        float result = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;
        
        for (int i = 0; i < 4; ++i)
        {
            int xi = (int)(x * frequency);
            int yi = (int)(y * frequency);
            float xf = (x * frequency) - xi;
            float yf = (y * frequency) - yi;
            
            float u = xf * xf * (3.0f - 2.0f * xf);
            float v = yf * yf * (3.0f - 2.0f * yf);
            
            auto hash = [](int px, int py, int s) -> float
            {
                uint64_t h = s;
                h ^= (uint64_t)(px * 0x9e3779b97f4a7c15ULL);
                h ^= (uint64_t)(py * 0xc2b2ae3d27d4eb4fULL);
                h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
                h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
                h = h ^ (h >> 31);
                return ((float)(h % 10000) / 10000.0f) * 2.0f - 1.0f;
            };
            
            float n00 = hash(xi, yi, seed + i);
            float n10 = hash(xi + 1, yi, seed + i);
            float n01 = hash(xi, yi + 1, seed + i);
            float n11 = hash(xi + 1, yi + 1, seed + i);
            
            float nx0 = n00 * (1.0f - u) + n10 * u;
            float nx1 = n01 * (1.0f - u) + n11 * u;
            float nxy = nx0 * (1.0f - v) + nx1 * v;
            
            result += nxy * amplitude;
            maxValue += amplitude;
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        
        return result / maxValue;
    };
    
    clear();
    
    std::vector<int> height(WORLD_COLS);
    int baseHeight = (int)(WORLD_ROWS * surfaceHeightFactor);
    float heightVariation = WORLD_ROWS * 0.08f * roughness;
    
    for (int c = 0; c < WORLD_COLS; ++c)
    {
        float noiseVal = perlin(c * 0.01f, seed * 0.0001f);
        height[c] = baseHeight + (int)(noiseVal * heightVariation);
        height[c] = Clamp(height[c], baseHeight / 2, baseHeight * 2);
    }
    
    int smoothPasses = (roughness < 1.5f) ? 3 : (roughness < 2.5f) ? 2 : 1;
    for (int pass = 0; pass < smoothPasses; ++pass)
    {
        std::vector<int> tmp = height;
        for (int c = 1; c < WORLD_COLS - 1; ++c)
            tmp[c] = (height[c-1] + height[c] + height[c+1]) / 3;
        height = tmp;
    }
    
    for (int r = 0; r < WORLD_ROWS; ++r)
    {
        for (int c = 0; c < WORLD_COLS; ++c)
        {
            Tile& t = tileAt(c, r);
            t.solid = true;
            
            int surfaceHeight = (c >= 0 && c < (int)height.size()) ? height[c] : baseHeight;
            
            if (r < surfaceHeight)
            {
                t.solid = false;
                t.color = WHITE;
            }
            else if (r < surfaceHeight + 10)
            {
                t.color = BROWN;
            }
            else
            {
                t.color = GRAY;
            }
        }
    }
    
    int caveBoundary = baseHeight + (int)(heightVariation * 1.5f);
    caveBoundary = Clamp(caveBoundary, baseHeight, WORLD_ROWS - 100);
    
    std::vector<float> caveNoise(WORLD_COLS * WORLD_ROWS);
    for (int r = 0; r < WORLD_ROWS; ++r)
    {
        for (int c = 0; c < WORLD_COLS; ++c)
        {
            float noiseVal = perlin(c * 0.02f, r * 0.015f);
            caveNoise[r * WORLD_COLS + c] = noiseVal;
        }
    }
    
    std::vector<bool> canBeCave(WORLD_COLS * WORLD_ROWS, false);
    float noiseThreshold = caveThreshold - (caveDensity - 1.0f) * 0.1f;
    noiseThreshold = Clamp(noiseThreshold, 0.05f, 0.8f);
    
    for (int r = 0; r < WORLD_ROWS; ++r)
    {
        for (int c = 0; c < WORLD_COLS; ++c)
        {
            int idx = r * WORLD_COLS + c;
            if (idx < 0 || idx >= (int)canBeCave.size()) continue;
            
            int distFromSurface = r - height[c];
            float depthFactor = distFromSurface > 0 ? Clamp((float)distFromSurface / 40.0f, 0.0f, 1.0f) : 0.0f;
            float surfaceThreshold = noiseThreshold - (1.0f - depthFactor) * caveSurfaceBias;
            surfaceThreshold = Clamp(surfaceThreshold, 0.0f, 1.0f);
            
            if (caveNoise[idx] > surfaceThreshold)
            {
                canBeCave[idx] = true;
            }
        }
    }
    
    int smoothIterations = Clamp(caveSmoothIterations, 1, 5);
    
    for (int iter = 0; iter < smoothIterations; ++iter)
    {
        std::vector<bool> newState = canBeCave;
        
        for (int r = 1; r < WORLD_ROWS - 1; ++r)
        {
            for (int c = 0; c < WORLD_COLS; ++c)
            {
                int idx = r * WORLD_COLS + c;
                if (idx < 0 || idx >= (int)canBeCave.size()) continue;
                
                int caveCount = 0;
                for (int dy = -1; dy <= 1; ++dy)
                {
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        if (dy == 0 && dx == 0) continue;
                        
                        int nr = r + dy;
                        int nc = c + dx;
                        
                        if (nc < 0) nc += WORLD_COLS;
                        if (nc >= WORLD_COLS) nc -= WORLD_COLS;
                        
                        if (nr >= 0 && nr < WORLD_ROWS)
                        {
                            int neighborIdx = nr * WORLD_COLS + nc;
                            if (neighborIdx >= 0 && neighborIdx < (int)canBeCave.size() && canBeCave[neighborIdx])
                                caveCount++;
                        }
                    }
                }
                
                bool isCave = canBeCave[idx];
                
                if (!isCave && caveCount >= 5)
                    newState[idx] = true;
                else if (isCave && caveCount <= 2)
                    newState[idx] = false;
            }
        }
        
        canBeCave = newState;
    }
    
    for (int r = 0; r < WORLD_ROWS; ++r)
    {
        for (int c = 0; c < WORLD_COLS; ++c)
        {
            int idx = r * WORLD_COLS + c;
            if (idx >= 0 && idx < (int)canBeCave.size() && canBeCave[idx])
            {
                tileAt(c, r).solid = false;
            }
        }
    }
    
    int numIsolatedCaves = (int)(20 * caveDensity);
    numIsolatedCaves = Clamp(numIsolatedCaves, 1, 100);
    
    for (int cave = 0; cave < numIsolatedCaves; ++cave)
    {
        int caveX = rng() % WORLD_COLS;
        int caveMaxY = WORLD_ROWS - 100;
        if (caveMaxY <= caveBoundary) caveMaxY = caveBoundary + 50;
        int caveY = caveBoundary + (rng() % (caveMaxY - caveBoundary));
        caveY = Clamp(caveY, caveBoundary, WORLD_ROWS - 10);
        int caveRadius = 2 + (rng() % 4);
        
        for (int dy = -caveRadius; dy <= caveRadius; ++dy)
        {
            for (int dx = -caveRadius; dx <= caveRadius; ++dx)
            {
                if ((rng() % 100) < 75)
                {
                    int dist2 = dx * dx + dy * dy;
                    if (dist2 <= caveRadius * caveRadius)
                    {
                        int cx = caveX + dx;
                        int cy = caveY + dy;
                        
                        if (cx < 0) cx += WORLD_COLS;
                        if (cx >= WORLD_COLS) cx -= WORLD_COLS;
                        
                        if (cy >= 0 && cy < WORLD_ROWS && inBounds(cx, cy))
                        {
                            int surfaceHeight = height[cx];
                            if (cy > surfaceHeight + 5)
                                tileAt(cx, cy).solid = false;
                        }
                    }
                }
            }
        }
    }
}

void TileMap::generateClouds(unsigned int seed, Vector2 spawnPos)
{
    int cloudLayerY = 80;
    int spawnTileX = (int)spawnPos.x / TILE_SIZE;
    int spawnTileY = (int)spawnPos.y / TILE_SIZE;
    
    // Keep the same cluster spawning mechanics, but make the silhouettes and shading softer.
    int numCloudClusters = 12;
    
    for (int clusterIdx = 0; clusterIdx < numCloudClusters; clusterIdx++)
    {
        std::mt19937 clusterRng(seed + clusterIdx * 54321);
        
        // Main cloud center position
        int centerX = (clusterRng() % WORLD_COLS);
        int centerY = cloudLayerY + (clusterRng() % 60) - 30; // wider Y range
        
        // Distance check: avoid spawning too close to player
        int distX = centerX - spawnTileX;
        int distY = centerY - spawnTileY;
        int distSq = distX * distX + distY * distY;
        if (distSq < 15000) continue;
        
        // Cloud shape: fluffy, organic with multiple overlapping puffs
        int numPuffs = 3 + (clusterRng() % 4); // 3-6 puffs per cloud
        int cloudWidthBase = 54 + (clusterRng() % 58);
        int cloudHeightBase = 18 + (clusterRng() % 24);
        
        // Create multiple overlapping ovals with a brighter top and softer underside
        for (int puffIdx = 0; puffIdx < numPuffs; puffIdx++)
        {
            std::mt19937 puffRng(seed + clusterIdx * 54321 + puffIdx * 98765);
            
            // Offset each puff slightly from center
            int puffOffX = (int)(puffRng() % (cloudWidthBase / 2)) - (cloudWidthBase / 4);
            int puffOffY = (int)(puffRng() % (cloudHeightBase / 2)) - (cloudHeightBase / 4);
            
            int puffCenterX = centerX + puffOffX;
            int puffCenterY = centerY + puffOffY;
            
            // Randomize puff size, biasing wider than tall for a cloud-like profile
            int puffRadiusX = 18 + (puffRng() % 20);
            int puffRadiusY = 9 + (puffRng() % 10);
            
            // Draw puff using elliptical falloff with soft top highlights and gray undersides
            for (int dy = -puffRadiusY; dy <= puffRadiusY; dy++)
            {
                for (int dx = -puffRadiusX; dx <= puffRadiusX; dx++)
                {
                    int tileX = puffCenterX + dx;
                    int tileY = puffCenterY + dy;
                    
                    if (!inBounds(tileX, tileY) || isSolid(tileX, tileY))
                        continue;
                    
                    float nx = (float)dx / (float)puffRadiusX;
                    float ny = (float)dy / (float)puffRadiusY;
                    float distFromPuffCenter = sqrtf(nx * nx + ny * ny);
                    float falloff = 1.0f - distFromPuffCenter;
                    
                    if (falloff > 0.0f)
                    {
                        // Add some noise to break up the perfect sphere
                        std::mt19937 noiseRng(seed + (tileX + 1) * 73856093U ^ (tileY + 1) * 19349663U);
                        float noiseFactor = 0.82f + (float)(noiseRng() % 100) / 350.0f;
                        falloff *= noiseFactor;
                        
                        if (falloff > 0.24f) // Only place cloud if density is visible
                        {
                            // Brighter toward the top, softer/greyer toward the underside
                            float topBias = Clamp(1.0f - ((float)(dy + puffRadiusY) / (float)(2 * puffRadiusY)), 0.0f, 1.0f);
                            float brightness = 210.0f + falloff * 45.0f + topBias * 25.0f;
                            brightness = Clamp(brightness, 190.0f, 255.0f);
                            unsigned char shade = (unsigned char)brightness;
                            unsigned char alpha = (unsigned char)Clamp(110.0f + falloff * 120.0f, 90.0f, 235.0f);
                            Color cloudColor = {shade, shade, (unsigned char)Clamp((float)shade + topBias * 6.0f, 0.0f, 255.0f), alpha};

                            tileAt(tileX, tileY) = {true, cloudColor};
                        }
                    }
                }
            }
        }
    }
}

void TileMap::draw(Rectangle visibleWorld) const
{
    int c0 = (int)(visibleWorld.x)                       / TILE_SIZE - 1;
    int r0 = (int)(visibleWorld.y)                       / TILE_SIZE - 1;
    int c1 = (int)(visibleWorld.x + visibleWorld.width)  / TILE_SIZE + 1;
    int r1 = (int)(visibleWorld.y + visibleWorld.height) / TILE_SIZE + 1;

    c0 = c0 < 0 ? 0 : c0;
    r0 = r0 < 0 ? 0 : r0;
    c1 = c1 >= WORLD_COLS ? WORLD_COLS - 1 : c1;
    r1 = r1 >= WORLD_ROWS ? WORLD_ROWS - 1 : r1;

    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++)
            if (isSolid(c, r))
                DrawRectangle(
                    c * TILE_SIZE, r * TILE_SIZE,
                    TILE_SIZE, TILE_SIZE,
                    tileAt(c, r).color
                );
}

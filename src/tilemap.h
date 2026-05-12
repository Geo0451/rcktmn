#pragma once
#include <vector>
#include <raylib.h>
#include "constants.h"

// ===== TILE DATA STRUCTURE =====

struct Tile
{
    bool solid = false;
    Color color = WHITE;
};

// ===== TILEMAP CLASS =====

class TileMap
{
public:
    std::vector<Tile> grid;

    TileMap();
    
    // Grid access
    bool inBounds(int col, int row) const;
    bool isSolid(int col, int row) const;
    Tile& tileAt(int col, int row);
    const Tile& tileAt(int col, int row) const;
    Rectangle getTileRect(int col, int row) const;
    int getSurfaceRow(int col) const;
    
    // Editing
    void fillPixelRect(int px, int py, int pw, int ph, bool solid, Color color = WHITE);
    void eraseAtPixel(int px, int py);
    void clear();
    
    // Generation
    void generate(unsigned int seed, float surfaceHeightFactor = 0.25f, float roughness = 1.0f,
                  float caveDensity = 1.0f, float caveThreshold = 0.4f,
                  float caveSurfaceBias = 0.22f, int caveSmoothIterations = 5);
    void generateClouds(unsigned int seed, Vector2 spawnPos);
    
    // Rendering
    void draw(Rectangle visibleWorld) const;
};

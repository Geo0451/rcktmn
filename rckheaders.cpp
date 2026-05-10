#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// ===== WORLD GRID CONSTANTS =====
const int TILE_SIZE  = 20;   // Matches player diameter (radius 10 * 2)
// World is 3000x3000 tiles (large open world)
const int WORLD_COLS = 3000; // World width  in tiles (60000px)
const int WORLD_ROWS = 3000; // World height in tiles (60000px)

// ===== MESSAGE =====

class Message
{
    public:
        float lifetime = 1.0f;
        std::string content = "Message";
        Color color = WHITE;
};

// ===== TILE MAP =====

struct Tile
{
    bool solid = false;
    Color color = WHITE;
};

class TileMap
{
    public:
        // We store tiles in a flat vector to avoid massive stack/static 2D arrays
        std::vector<Tile> grid;

        TileMap()
        {
            grid.resize((size_t)WORLD_ROWS * (size_t)WORLD_COLS);
            for (size_t i = 0; i < grid.size(); ++i) grid[i] = {false, WHITE};
        }

        inline bool inBounds(int col, int row) const
        {
            return col >= 0 && col < WORLD_COLS && row >= 0 && row < WORLD_ROWS;
        }

        inline bool isSolid(int col, int row) const
        {
            if (!inBounds(col, row)) return false;
            return grid[(size_t)row * WORLD_COLS + (size_t)col].solid;
        }

        inline Tile& tileAt(int col, int row)
        {
            return grid[(size_t)row * WORLD_COLS + (size_t)col];
        }

        inline const Tile& tileAt(int col, int row) const
        {
            return grid[(size_t)row * WORLD_COLS + (size_t)col];
        }

        Rectangle getTileRect(int col, int row) const
        {
            return {
                (float)(col * TILE_SIZE),
                (float)(row * TILE_SIZE),
                (float)TILE_SIZE,
                (float)TILE_SIZE
            };
        }

        // Return the surface (first solid row) for a given column, or WORLD_ROWS if none
        int getSurfaceRow(int col) const
        {
            if (col < 0 || col >= WORLD_COLS) return WORLD_ROWS;
            for (int r = 0; r < WORLD_ROWS; ++r)
                if (isSolid(col, r)) return r;
            return WORLD_ROWS;
        }

        // Fill a region defined by world-pixel coordinates with solid tiles
        void fillPixelRect(int px, int py, int pw, int ph, bool solid, Color color = WHITE)
        {
            // Ensure at least one tile is covered (handles single-click px=pw=0)
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

        void eraseAtPixel(int px, int py)
        {
            int c = px / TILE_SIZE;
            int r = py / TILE_SIZE;
            if (inBounds(c, r)) tileAt(c, r) = {false, WHITE};
        }

        void clear()
        {
            for (size_t i = 0; i < grid.size(); ++i)
                grid[i] = {false, WHITE};
        }

        // Wave Function Collapse-based world generation
         void generate(unsigned int seed, float surfaceHeightFactor = 0.25f, float roughness = 1.0f,
                       float caveDensity = 1.0f, float caveThreshold = 0.4f,
                       float caveSurfaceBias = 0.22f, int caveSmoothIterations = 5)
         {
             std::mt19937 rng(seed);
             
             // Perlin-like noise function using hash
             auto perlin = [&](float x, float y) -> float
             {
                 // Simple improved perlin noise approximation using multiple octaves
                 float result = 0.0f;
                 float amplitude = 1.0f;
                 float frequency = 1.0f;
                 float maxValue = 0.0f;
                 
                 for (int i = 0; i < 4; ++i)
                 {
                     // Hash-based pseudo-random gradient
                     int xi = (int)(x * frequency);
                     int yi = (int)(y * frequency);
                     float xf = (x * frequency) - xi;
                     float yf = (y * frequency) - yi;
                     
                     // Fade function (smoothstep)
                     float u = xf * xf * (3.0f - 2.0f * xf);
                     float v = yf * yf * (3.0f - 2.0f * yf);
                     
                     // Hash values for corners
                     auto hash = [](int px, int py, int seed) -> float
                     {
                         uint64_t h = seed;
                         h ^= (uint64_t)(px * 0x9e3779b97f4a7c15ULL);
                         h ^= (uint64_t)(py * 0xc2b2ae3d27d4eb4fULL);
                         h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
                         h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
                         h = h ^ (h >> 31);
                         return ((float)(h % 10000) / 10000.0f) * 2.0f - 1.0f;
                     };
                     
                     // Get values at 4 corners
                     float n00 = hash(xi, yi, seed + i);
                     float n10 = hash(xi + 1, yi, seed + i);
                     float n01 = hash(xi, yi + 1, seed + i);
                     float n11 = hash(xi + 1, yi + 1, seed + i);
                     
                     // Interpolate
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
             
             // Generate surface using Perlin noise
             std::vector<int> height(WORLD_COLS);
             int baseHeight = (int)(WORLD_ROWS * surfaceHeightFactor);
             float heightVariation = WORLD_ROWS * 0.08f * roughness;
             
             for (int c = 0; c < WORLD_COLS; ++c)
             {
                 float noiseVal = perlin(c * 0.01f, seed * 0.0001f);
                 height[c] = baseHeight + (int)(noiseVal * heightVariation);
                 height[c] = Clamp(height[c], baseHeight / 2, baseHeight * 2);
             }
             
             // Extra smoothing passes based on roughness
             int smoothPasses = (roughness < 1.5f) ? 3 : (roughness < 2.5f) ? 2 : 1;
             for (int pass = 0; pass < smoothPasses; ++pass)
             {
                 std::vector<int> tmp = height;
                 for (int c = 1; c < WORLD_COLS - 1; ++c)
                     tmp[c] = (height[c-1] + height[c] + height[c+1]) / 3;
                 height = tmp;
             }
             
             // STEP 1: Fill entire world as solid terrain with layers
             for (int r = 0; r < WORLD_ROWS; ++r)
             {
                 for (int c = 0; c < WORLD_COLS; ++c)
                 {
                     Tile& t = tileAt(c, r);
                     t.solid = true;
                     
                     // Color based on depth
                     if (r < height[c])
                     {
                         // Above surface - air
                         t.solid = false;
                         t.color = WHITE;
                     }
                     else if (r < height[c] + 10)
                     {
                         // Dirt layer
                         t.color = BROWN;
                     }
                     else
                     {
                         // Stone layer
                         t.color = GRAY;
                     }
                 }
             }
             
             // STEP 2: Generate cave pattern using organic wave function collapse
             int caveBoundary = baseHeight + (int)(heightVariation * 1.5f);
             
             // Use Perlin noise to seed initial cave regions for organic shape
             std::vector<float> caveNoise(WORLD_COLS * WORLD_ROWS);
             for (int r = 0; r < WORLD_ROWS; ++r)
             {
                 for (int c = 0; c < WORLD_COLS; ++c)
                 {
                     float noiseVal = perlin(c * 0.02f, r * 0.015f);
                     caveNoise[r * WORLD_COLS + c] = noiseVal;
                 }
             }
             
             // Initialize cave state based on Perlin noise threshold
             std::vector<bool> canBeCave(WORLD_COLS * WORLD_ROWS, false);
             float noiseThreshold = caveThreshold - (caveDensity - 1.0f) * 0.1f;
             noiseThreshold = Clamp(noiseThreshold, 0.0f, 0.6f);
             
             for (int r = 0; r < WORLD_ROWS; ++r)
             {
                 for (int c = 0; c < WORLD_COLS; ++c)
                 {
                     int distFromSurface = r - height[c];
                     float depthFactor = Clamp((float)distFromSurface / 40.0f, 0.0f, 1.0f);
                     float surfaceThreshold = noiseThreshold - (1.0f - depthFactor) * caveSurfaceBias;
                     
                     // Carve caves based on noise across the full world so they can overlap the surface
                     if (caveNoise[r * WORLD_COLS + c] > surfaceThreshold)
                     {
                         canBeCave[r * WORLD_COLS + c] = true;
                     }
                 }
             }
             
             // Cellular automata smoothing for organic connectivity
             int smoothIterations = Clamp(caveSmoothIterations, 1, 12);
             
             for (int iter = 0; iter < smoothIterations; ++iter)
             {
                 std::vector<bool> newState = canBeCave;
                 
                 for (int r = 1; r < WORLD_ROWS - 1; ++r)
                 {
                     for (int c = 0; c < WORLD_COLS; ++c)
                     {
                         // Check 8 neighbors (Moore neighborhood)
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
                                 
                                 if (nr >= 0 && nr < WORLD_ROWS && canBeCave[nr * WORLD_COLS + nc])
                                     caveCount++;
                             }
                         }
                         
                         bool isCave = canBeCave[r * WORLD_COLS + c];
                         
                         // Cave rules for connectivity and erosion
                         if (!isCave && caveCount >= 5)
                             newState[r * WORLD_COLS + c] = true;
                         else if (isCave && caveCount <= 2)
                             newState[r * WORLD_COLS + c] = false;
                     }
                 }
                 
                 canBeCave = newState;
             }
             
             // STEP 3: Apply cave pattern as overlay - carve out any solid tiles marked as caves
             for (int r = 0; r < WORLD_ROWS; ++r)
             {
                 for (int c = 0; c < WORLD_COLS; ++c)
                 {
                     if (canBeCave[r * WORLD_COLS + c])
                     {
                         tileAt(c, r).solid = false;
                     }
                 }
             }
             
             // Add some small isolated caves for variety
             int numIsolatedCaves = (int)(20 * caveDensity);
             
             for (int cave = 0; cave < numIsolatedCaves; ++cave)
             {
                 int caveX = rng() % WORLD_COLS;
                 int caveY = caveBoundary + (rng() % (WORLD_ROWS - caveBoundary - 50));
                 int caveRadius = 2 + (rng() % 4);
                 
                 // Carve an organic-looking cave
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
                                 
                                 if (cy >= 0 && cy < WORLD_ROWS && cy > height[cx] + 5)
                                     tileAt(cx, cy).solid = false;
                             }
                         }
                     }
                 }
             }
         }        // Only draw tiles in the visible rect (viewport culling)
        void draw(Rectangle visibleWorld) const
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
};

// ===== PLAYER =====

class Player
{
    public:
        Vector2 pos;
        Vector2 dir;
        float speed;
        float size;       // Radius
        Color clr;

        float x_friction;
        float x_max_vel, y_max_vel;
        bool canJump;

        Vector2 updated_pos(float dt)
        {
            dt *= 60;

            dir.x = Clamp(dir.x, -x_max_vel, +x_max_vel);
            dir.y = Clamp(dir.y, -y_max_vel * 1.5f, +y_max_vel * 2.0f);

            if (dir.x < 0.1f && dir.x > -0.1f)
                dir.x = 0;
            else
                dir.x = dir.x / x_friction;

            return {
                pos.x + (dir.x * speed * dt),
                pos.y + (dir.y * speed * dt)
            };
        }

        // Border checking removed — world is now virtually unbounded (large world)
};

// ===== MAKER MODE =====

class Make
{
    public:
        Vector2 point_a = {0, 0};
        Vector2 point_b = {0, 0};
        bool dragging   = false;
        
        // Block type: 0 = dirt, 1 = stone, 2 = erase
        int blockType = 0;
        bool showMenu = false;

        // Raw drag rect in world space
        Rectangle getDragRect() const
        {
            float x = (point_a.x < point_b.x) ? point_a.x : point_b.x;
            float y = (point_a.y < point_b.y) ? point_a.y : point_b.y;
            float w = fabsf(point_b.x - point_a.x);
            float h = fabsf(point_b.y - point_a.y);
            return {x, y, w, h};
        }

        // Drag rect snapped to tile grid (visual ghost)
        Rectangle getSnappedRect() const
        {
            Rectangle drag = getDragRect();
            int c0 = (int)drag.x / TILE_SIZE;
            int r0 = (int)drag.y / TILE_SIZE;
            int c1 = ((int)(drag.x + drag.width))  / TILE_SIZE;
            int r1 = ((int)(drag.y + drag.height)) / TILE_SIZE;
            return {
                (float)(c0 * TILE_SIZE),
                (float)(r0 * TILE_SIZE),
                (float)((c1 - c0 + 1) * TILE_SIZE),
                (float)((r1 - r0 + 1) * TILE_SIZE)
            };
        }

        // worldMouse = GetScreenToWorld2D(GetMousePosition(), camera)
        void select(TileMap& tileMap, Vector2 worldMouse)
        {
            // B key toggles block type menu
            if (IsKeyPressed(KEY_B))
                showMenu = !showMenu;

            // Keyboard shortcuts in menu
            if (showMenu)
            {
                if (IsKeyPressed(KEY_ONE)) { blockType = 0; showMenu = false; }
                if (IsKeyPressed(KEY_TWO)) { blockType = 1; showMenu = false; }
                if (IsKeyPressed(KEY_THREE)) { blockType = 2; showMenu = false; }
                if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_ESCAPE)) showMenu = false;
                return;
            }

            // Left-click: start drag for placing blocks
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                point_a  = worldMouse;
                point_b  = worldMouse;
                dragging = true;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && dragging)
                point_b = worldMouse;

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && dragging)
            {
                point_b = worldMouse;
                Rectangle drag = getDragRect();
                
                if (blockType == 2) // erase
                {
                    tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                          (int)drag.width, (int)drag.height,
                                          false, WHITE);
                }
                else if (blockType == 0) // dirt
                {
                    tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                          (int)drag.width, (int)drag.height,
                                          true, BROWN);
                }
                else if (blockType == 1) // stone
                {
                    tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                          (int)drag.width, (int)drag.height,
                                          true, GRAY);
                }
                dragging = false;
            }

            // Right-click: drag to erase (same as left-click with erase mode)
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
            {
                point_a  = worldMouse;
                point_b  = worldMouse;
                dragging = true;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && dragging)
                point_b = worldMouse;

            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && dragging)
            {
                point_b = worldMouse;
                Rectangle drag = getDragRect();
                tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                      (int)drag.width, (int)drag.height,
                                      false, WHITE);
                dragging = false;
            }
        }

        // Call this inside BeginMode2D so coordinates match the world
        void drawPreview(Vector2 worldMouse) const
        {
            // Tile under cursor highlight
            int hc = (int)worldMouse.x / TILE_SIZE;
            int hr = (int)worldMouse.y / TILE_SIZE;
            if (hc >= 0 && hc < WORLD_COLS && hr >= 0 && hr < WORLD_ROWS)
            {
                Rectangle hover = {
                    (float)(hc * TILE_SIZE),
                    (float)(hr * TILE_SIZE),
                    (float)TILE_SIZE,
                    (float)TILE_SIZE
                };
                DrawRectangleLinesEx(hover, 1, Fade(GREEN, 0.5f));
            }

            // Drag ghost
            if (dragging)
            {
                Rectangle snapped = getSnappedRect();
                Color ghostColor = (blockType == 2) ? RED : GREEN;
                DrawRectangle((int)snapped.x, (int)snapped.y,
                              (int)snapped.width, (int)snapped.height,
                              Fade(ghostColor, 0.2f));
                DrawRectangleLinesEx(snapped, 1, ghostColor);
            }
        }

        // Draw block type menu in screen space
        void drawMenu() const
        {
            int scrW = 1920, scrH = 1080;
            int menuW = 400, menuH = 200;
            int x = (scrW - menuW) / 2;
            int y = (scrH - menuH) / 2;

            DrawRectangle(x, y, menuW, menuH, Fade(BLACK, 0.8f));
            DrawRectangleLinesEx({(float)x, (float)y, (float)menuW, (float)menuH}, 2, WHITE);

            DrawText("BLOCK TYPE (1-3 or Click)", x + 20, y + 20, 20, YELLOW);

            // Option 1: Dirt
            Color c1 = (blockType == 0) ? YELLOW : WHITE;
            DrawText("1. Dirt", x + 40, y + 60, 20, c1);
            DrawRectangle(x + 250, y + 60, 30, 20, BROWN);

            // Option 2: Stone
            Color c2 = (blockType == 1) ? YELLOW : WHITE;
            DrawText("2. Stone", x + 40, y + 100, 20, c2);
            DrawRectangle(x + 250, y + 100, 30, 20, GRAY);

            // Option 3: Erase
            Color c3 = (blockType == 2) ? YELLOW : WHITE;
            DrawText("3. Erase", x + 40, y + 140, 20, c3);
            DrawRectangle(x + 250, y + 140, 30, 20, BLACK);
            DrawRectangleLinesEx({x + 250.0f, y + 140.0f, 30.0f, 20.0f}, 1, WHITE);

            DrawText("Press B to close", x + 20, y + 170, 16, DARKGRAY);
        }
};

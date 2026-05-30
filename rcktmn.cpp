#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <raylib.h>

#include "src/constants.h"
#include "src/ui.h"
#include "src/tilemap.h"
#include "src/player.h"
#include "src/maker.h"
#include "src/gameplay.h"

// Local configuration
const int scrWidth  = SCREEN_WIDTH;
const int scrHeight = SCREEN_HEIGHT;
const int fpsMax    = FPS_MAX;
const int MAX_SLOTS = MAX_SAVE_SLOTS;

// Helper: Clamp function (if not available from raylib)
template<typename T>
T Clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

static bool sameColor(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static int minedBlockTypeFromTile(const Tile& tile)
{
    if (sameColor(tile.color, BROWN)) return 0;
    if (sameColor(tile.color, GRAY)) return 1;
    return -1;
}

static Color selectedInventoryBlockColor(const Inventory& inventory)
{
    if (inventory.blockCounts[inventory.selectedSlot] <= 0)
        return WHITE;

    if (inventory.selectedSlot == 0)
        return BROWN;

    if (inventory.selectedSlot == 1)
        return GRAY;

    return WHITE;
}

// World dimensions in pixels
const int worldWidth  = WORLD_COLS * TILE_SIZE;
const int worldHeight = WORLD_ROWS * TILE_SIZE;

int main()
{
    srand(5894);

    Make tileMaker;
    bool makerMode = false;  // Survival mode is now default

    int currentSlot = 1;

    enum SaveLoadMode { NONE, SAVING, LOADING };
    enum GameState { MENU, PREVIEW, PLAYING };
    SaveLoadMode pauseMode = NONE;
    GameState gameState = MENU;
    
    bool showInventory = false;
    float miningHeldTime = 0.0f;
    const float MINING_TIME_REQUIRED = 0.3f;
    
    Inventory inventory;

    bool slotHasData[MAX_SLOTS + 1] = {};

    std::vector<Message> consoleMessages;
    consoleMessages.push_back({
        7.0f,
        "WELCOME 2 RCKTMN\n WASD:MOVE  W/SPACE:JUMP  SCROLL:ZOOM\n CTRL-M:MAKER  L:DRAW  R:ERASE\n CTRL-S/L:SAVE/LOAD"
    });

    Player player = {
        {scrWidth / 2.0f, scrHeight / 2.0f},
        {0.0f, 0.0f},
        2.0f,
        10.0f,  // Radius - tile size = diameter = 20px
        RED,
        1.05f,
        2.8f,
        3.0f,
        false
    };

    TileMap tileMap;
    unsigned int seedVal = 0;
    std::string seedInput = "";

    // World generation parameters
    float surfaceHeight = 0.25f;        // 0.0-1.0, where on screen is surface
    float surfaceRoughness = 1.0f;      // 1.0-5.0, higher = more jagged
    float caveDensity = 1.0f;           // 0.1-5.0, how many caves (0.1=rare, 5.0=very dense)
    float caveThreshold = 0.40f;        // 0.05-0.80, higher = fewer caves
    float caveSurfaceBias = 0.22f;      // 0.00-0.50, higher = more surface overlap
    int caveSmoothIterations = 5;       // 1-12, more = smoother/connected caves

    // ----- Camera setup -----
    Camera2D camera    = {};
    camera.offset      = {scrWidth / 2.0f, scrHeight / 2.0f};
    camera.target      = {scrWidth / 2.0f, scrHeight / 2.0f};
    camera.rotation    = 0.0f;
    camera.zoom        = 1.0f;

    InitWindow(scrWidth, scrHeight, "RCKTMN");
    SetTargetFPS(fpsMax);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // === MENU STATE ===
        if (gameState == MENU)
        {
            // Handle seed input
            if (IsKeyPressed(KEY_BACKSPACE) && !seedInput.empty())
                seedInput.pop_back();

            int key = GetCharPressed();
            while (key > 0)
            {
                if ((key >= '0' && key <= '9') || key == '-')
                    if (seedInput.length() < 10) seedInput += (char)key;
                key = GetCharPressed();
            }

            // Start generation on Enter
            if (IsKeyPressed(KEY_ENTER))
            {
                if (seedInput.empty())
                {
                    std::random_device rd;
                    seedVal = (unsigned int)rd();
                }
                else
                {
                    try { seedVal = (unsigned int)std::stoul(seedInput); }
                    catch (...) { seedVal = 0; }
                }
                gameState = PREVIEW;
                tileMap.generate(seedVal, surfaceHeight, surfaceRoughness, caveDensity, caveThreshold, caveSurfaceBias, caveSmoothIterations);
                tileMap.generateClouds(seedVal, player.pos);
            }

            // Render menu
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("RCKTMN - WORLD GENERATOR", scrWidth/2 - 250, 100, 40, WHITE);
            DrawText("Enter Seed:", 300, 300, 30, YELLOW);
            
            // Input box
            DrawRectangleLinesEx({250.0f, 350.0f, 500.0f, 50.0f}, 2, WHITE);
            DrawText(seedInput.c_str(), 270, 365, 30, WHITE);
            DrawText("(Leave empty for random seed)", 300, 420, 20, DARKGRAY);
            DrawText("Press ENTER to generate world", 300, 480, 20, DARKGRAY);

            EndDrawing();
            continue;
        }

        // === PREVIEW STATE ===
        if (gameState == PREVIEW)
        {
            // Slider interaction
            Vector2 mouse = GetMousePosition();
            const float labelX = 50.0f;
            const float sliderX = 280.0f;
            const float sliderW = 300.0f;
            const float sliderH = 20.0f;
            const float sliderSpacing = 60.0f;
            const float valueX = sliderX + sliderW + 30.0f;
            float sliderY = 150.0f;

            // Helper lambda to draw and interact with sliders
            auto drawSlider = [&](const char* label, float& value, float minVal, float maxVal, float sliderYPos) -> void
            {
                // Label on left
                DrawText(label, (int)labelX, (int)sliderYPos, 16, WHITE);
                
                // Slider bar (clickable area)
                Rectangle sliderBar = {sliderX, sliderYPos, sliderW, sliderH};
                DrawRectangleRec(sliderBar, DARKGRAY);
                DrawRectangleLinesEx(sliderBar, 2, WHITE);
                
                // Slider knob position
                float t = (value - minVal) / (maxVal - minVal);
                t = Clamp(t, 0.0f, 1.0f);
                float knobX = sliderX + t * sliderW;
                
                // Check for click/drag anywhere on slider bar
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, sliderBar))
                {
                    float newT = (mouse.x - sliderX) / sliderW;
                    newT = Clamp(newT, 0.0f, 1.0f);
                    value = minVal + newT * (maxVal - minVal);
                }
                
                // Draw knob
                DrawCircle((int)knobX, (int)(sliderYPos + sliderH * 0.5f), 8, YELLOW);
                
                // Value display on right
                DrawText(TextFormat("%.2f", value), (int)valueX, (int)sliderYPos, 16, LIGHTGRAY);
            };

            auto drawIntSlider = [&](const char* label, int& value, int minVal, int maxVal, float sliderYPos) -> void
            {
                // Label on left
                DrawText(label, (int)labelX, (int)sliderYPos, 16, WHITE);
                
                // Slider bar (clickable area)
                Rectangle sliderBar = {sliderX, sliderYPos, sliderW, sliderH};
                DrawRectangleRec(sliderBar, DARKGRAY);
                DrawRectangleLinesEx(sliderBar, 2, WHITE);
                
                // Slider knob position
                float t = (float)(value - minVal) / (float)(maxVal - minVal);
                t = Clamp(t, 0.0f, 1.0f);
                float knobX = sliderX + t * sliderW;
                
                // Check for click/drag anywhere on slider bar
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, sliderBar))
                {
                    float newT = (mouse.x - sliderX) / sliderW;
                    newT = Clamp(newT, 0.0f, 1.0f);
                    value = (int)(minVal + newT * (float)(maxVal - minVal));
                }
                
                // Draw knob
                DrawCircle((int)knobX, (int)(sliderYPos + sliderH * 0.5f), 8, YELLOW);
                
                // Value display on right
                DrawText(TextFormat("%d", value), (int)valueX, (int)sliderYPos, 16, LIGHTGRAY);
            };

            // Wait for ENTER to start game
            if (IsKeyPressed(KEY_ENTER))
            {
                gameState = PLAYING;
                // Place player on surface
                int centerCol = WORLD_COLS / 2;
                int surfRow = tileMap.getSurfaceRow(centerCol);
                if (surfRow < WORLD_ROWS)
                {
                    player.pos.x = centerCol * TILE_SIZE + TILE_SIZE * 0.5f;
                    player.pos.y = surfRow * TILE_SIZE - player.size - 1.0f;
                }
                camera.target = player.pos;
            }

            // ESC to go back to menu
            if (IsKeyPressed(KEY_ESCAPE))
            {
                gameState = MENU;
                seedInput = "";
                continue;
            }

            // Render preview
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("WORLD PREVIEW & GENERATION TUNING", scrWidth/2 - 300, 20, 40, YELLOW);

            // Draw sliders on left side
            DrawText("=== GENERATION PARAMETERS ===", (int)labelX, (int)(sliderY - 40), 20, SKYBLUE);
            
            drawSlider("Surface Height", surfaceHeight, 0.1f, 0.5f, sliderY);
            sliderY += sliderSpacing;
            
            drawSlider("Surface Roughness", surfaceRoughness, 0.5f, 3.0f, sliderY);
            sliderY += sliderSpacing;
            
            drawSlider("Cave Density", caveDensity, 0.1f, 5.0f, sliderY);
            sliderY += sliderSpacing;

            drawSlider("Cave Threshold", caveThreshold, 0.05f, 0.80f, sliderY);
            sliderY += sliderSpacing;

            drawSlider("Surface Bias", caveSurfaceBias, 0.00f, 0.50f, sliderY);
            sliderY += sliderSpacing;

            drawIntSlider("Collapse Iterations", caveSmoothIterations, 1, 12, sliderY);
            sliderY += sliderSpacing;

            // Check for regenerate button
            Rectangle regenBtn = {labelX, sliderY + 10.0f, 220.0f, 40.0f};
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, regenBtn))
            {
                tileMap.generate(seedVal, surfaceHeight, surfaceRoughness, caveDensity, caveThreshold, caveSurfaceBias, caveSmoothIterations);
                tileMap.generateClouds(seedVal, player.pos);
            }
            
            // Also allow R key to regenerate
            if (IsKeyPressed(KEY_R))
            {
                tileMap.generate(seedVal, surfaceHeight, surfaceRoughness, caveDensity, caveThreshold, caveSurfaceBias, caveSmoothIterations);
                tileMap.generateClouds(seedVal, player.pos);
            }

            // Regenerate button
            bool regenerateHovered = CheckCollisionPointRec(mouse, regenBtn);
            DrawRectangleRec(regenBtn, regenerateHovered ? ORANGE : DARKGREEN);
            DrawRectangleLinesEx(regenBtn, 2, regenerateHovered ? YELLOW : WHITE);
            DrawText("REGENERATE (R)", (int)(regenBtn.x + 15), (int)(regenBtn.y + 10), 16, WHITE);

            // Draw world preview on right side
            float previewW = 700.0f;
            float previewH = 700.0f;
            float previewX = scrWidth - previewW - 50.0f;
            float previewY = 120.0f;
            float scaleX = previewW / (WORLD_COLS * TILE_SIZE);
            float scaleY = previewH / (WORLD_ROWS * TILE_SIZE);

            // Render world preview
            for (int r = 0; r < WORLD_ROWS; r += 5)
            {
                for (int c = 0; c < WORLD_COLS; c += 5)
                {
                    if (tileMap.isSolid(c, r))
                    {
                        float px = previewX + c * TILE_SIZE * scaleX;
                        float py = previewY + r * TILE_SIZE * scaleY;
                        float pw = TILE_SIZE * scaleX + 1.0f;
                        float ph = TILE_SIZE * scaleY + 1.0f;
                        DrawRectangle((int)px, (int)py, (int)pw, (int)ph, tileMap.tileAt(c, r).color);
                    }
                }
            }
            DrawRectangleLinesEx({previewX, previewY, previewW, previewH}, 2, WHITE);

            DrawText(("Seed: " + std::to_string(seedVal)).c_str(), 50, scrHeight - 100, 20, DARKGRAY);
            DrawText("Press ENTER to start playing", 50, scrHeight - 60, 20, GREEN);
            DrawText("Press ESC to change seed", 50, scrHeight - 30, 20, LIGHTGRAY);

            EndDrawing();
            continue;
        }

        // === PLAYING STATE ===
        float wheel = GetMouseWheelMove();
        if (pauseMode == NONE && !showInventory && wheel != 0.0f)
        {
            camera.zoom += wheel * ZOOM_STEP;
            camera.zoom = Clamp(camera.zoom, ZOOM_MIN, ZOOM_MAX);
        }

        // World-space mouse position (needed for maker mode under any zoom level)
        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);

        // Visible world rect (for draw culling)
        Rectangle visibleWorld = {
            camera.target.x - (scrWidth  / 2.0f) / camera.zoom,
            camera.target.y - (scrHeight / 2.0f) / camera.zoom,
            (float)scrWidth  / camera.zoom,
            (float)scrHeight / camera.zoom
        };

        // ----- INPUT -----
        if (pauseMode == NONE)
        {
            if (!makerMode && !showInventory)
                handlePlayerInput(player);
            
            // Survival mode: TAB toggles inventory
            if (!makerMode && IsKeyPressed(KEY_TAB))
            {
                showInventory = !showInventory;
                miningHeldTime = 0.0f;
            }
            
            // Survival mode: direct Minecraft-style controls
            // - LMB hold to mine and break blocks
            // - RMB places the selected block if the target tile is empty
            if (!makerMode && !showInventory)
            {
                // Left click hold: accumulate mining time
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    miningHeldTime += deltaTime;

                    // Break the block once mining is complete
                    if (miningHeldTime >= MINING_TIME_REQUIRED)
                    {
                        int tx = (int)worldMouse.x / TILE_SIZE;
                        int ty = (int)worldMouse.y / TILE_SIZE;

                        if (tileMap.inBounds(tx, ty) && tileMap.isSolid(tx, ty))
                        {
                            Tile& tile = tileMap.tileAt(tx, ty);
                            int blockType = minedBlockTypeFromTile(tile);

                            tileMap.tileAt(tx, ty).solid = false;
                            if (blockType >= 0)
                                inventory.addBlock(blockType, 1);
                            consoleMessages.push_back({1.0f, "MINED BLOCK", YELLOW});
                            miningHeldTime = 0.0f;  // Reset for next mine
                        }
                    }
                }
                else
                {
                    miningHeldTime = 0.0f;  // Reset when mouse is released
                }

                // Right click: place selected block immediately
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                {
                    int tx = (int)worldMouse.x / TILE_SIZE;
                    int ty = (int)worldMouse.y / TILE_SIZE;

                    if (tileMap.inBounds(tx, ty) && !tileMap.isSolid(tx, ty))
                    {
                        if (inventory.getBlockCount(inventory.selectedSlot) > 0)
                        {
                            if (inventory.removeBlock(inventory.selectedSlot, 1))
                            {
                                Color blockColor = (inventory.selectedSlot == 0) ? BROWN : GRAY;
                                tileMap.tileAt(tx, ty) = {true, blockColor};
                                consoleMessages.push_back({0.5f, "PLACED BLOCK", GREEN});
                            }
                        }
                    }
                }

                // Number keys to select slot
                for (int i = 0; i < 9; i++)
                {
                    if (IsKeyPressed(KEY_ONE + i)) inventory.selectedSlot = i;
                }
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
            {
                pauseMode = SAVING;
                for (int i = 1; i <= MAX_SLOTS; i++)
                {
                    std::ifstream f("sf_" + std::to_string(i) + ".dat");
                    slotHasData[i] = f.is_open();
                }
                consoleMessages.push_back({5.0f, "SELECT SLOT 2 SAVE (1-5)", YELLOW});
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_L))
            {
                pauseMode = LOADING;
                for (int i = 1; i <= MAX_SLOTS; i++)
                {
                    std::ifstream f("sf_" + std::to_string(i) + ".dat");
                    slotHasData[i] = f.is_open();
                }
                consoleMessages.push_back({5.0f, "SELECT SLOT 2 LOAD (1-5)", YELLOW});
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_M))
            {
                makerMode = !makerMode;
                if (makerMode)
                {
                    showInventory = false;
                    miningHeldTime = 0.0f;
                    player.flying = true;
                    player.flyVelX = 0.0f;
                    player.flyVelY = 0.0f;
                    consoleMessages.push_back(Message{2.0f, "MAKER MODE ON  B:BLOCKS  L:DRAG  R:ERASE  CTRL-M:OFF", WHITE});
                }
                else
                {
                    player.flying = false;
                    player.flyVelX = 0.0f;
                    miningHeldTime = 0.0f;
                    consoleMessages.push_back(Message{1.0f, "MAKER MODE OFF", WHITE});
                }
            }

            // Creative flight controls (only in maker mode)
            if (makerMode && player.flying)
            {
                float flyAccel = 8.0f;  // Flight acceleration
                if (IsKeyDown(KEY_A))
                {
                    player.flyVelX -= flyAccel;  // Negative X = left
                }
                if (IsKeyDown(KEY_D))
                {
                    player.flyVelX += flyAccel;  // Positive X = right
                }
                if (IsKeyDown(KEY_W))
                {
                    player.flyVelY -= flyAccel;  // Negative Y = up
                }
                if (IsKeyDown(KEY_S))
                {
                    player.flyVelY += flyAccel;  // Positive Y = down
                }
            }

            // Block type hotkeys (only in maker mode, when menu not open)
            if (makerMode && !tileMaker.showMenu)
            {
                if (IsKeyPressed(KEY_ONE)) tileMaker.blockType = 0;
                if (IsKeyPressed(KEY_TWO)) tileMaker.blockType = 1;
                if (IsKeyPressed(KEY_THREE)) tileMaker.blockType = 2;
            }

            if (makerMode)
                tileMaker.select(tileMap, worldMouse);
        }
        else
        {
            int selectedSlot = 0;
            if (IsKeyPressed(KEY_ONE))   selectedSlot = 1;
            else if (IsKeyPressed(KEY_TWO))   selectedSlot = 2;
            else if (IsKeyPressed(KEY_THREE)) selectedSlot = 3;
            else if (IsKeyPressed(KEY_FOUR))  selectedSlot = 4;
            else if (IsKeyPressed(KEY_FIVE))  selectedSlot = 5;

            if (selectedSlot > 0)
            {
                currentSlot = selectedSlot;
                if (pauseMode == SAVING)
                {
                    saveLevelToSlot(tileMap, player, currentSlot);
                    consoleMessages.push_back({1.5f, "SAVED TO SLOT " + std::to_string(currentSlot), GREEN});
                }
                else if (pauseMode == LOADING)
                {
                    if (loadLevelFromSlot(tileMap, player, currentSlot))
                        consoleMessages.push_back({1.5f, "LOADED FROM SLOT " + std::to_string(currentSlot), GREEN});
                    else
                        consoleMessages.push_back({1.5f, "SLOT " + std::to_string(currentSlot) + " EMPTY", RED});
                }
                pauseMode = NONE;
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                consoleMessages.push_back({1.0f, "CANCELLED", RED});
                pauseMode = NONE;
            }
        }

        // ----- PHYSICS -----
        if (pauseMode == NONE && !showInventory)
        {
            player.dir.y += GRAVITY;
            Vector2 nextPos = player.updated_pos(deltaTime);
            handleCollisions(player, nextPos, tileMap);
        }

        // Camera follows the player in normal play; maker mode can zoom all the way out to a full-map overview
        if (makerMode)
        {
            // At minimum zoom, center on world; otherwise follow player
            if (camera.zoom <= ZOOM_MIN + 0.001f)
                camera.target = {(float)worldWidth * 0.5f, (float)worldHeight * 0.5f};
            else
                camera.target = player.pos;
        }
        else
        {
            camera.target = player.pos;
        }
        
        // Clamp camera to world bounds
        float camWidth = (float)scrWidth / camera.zoom;
        float camHeight = (float)scrHeight / camera.zoom;
        camera.target.x = Clamp(camera.target.x, camWidth * 0.5f, (float)worldWidth - camWidth * 0.5f);
        camera.target.y = Clamp(camera.target.y, camHeight * 0.5f, (float)worldHeight - camHeight * 0.5f);

        // ----- RENDERING -----
        // Precompute player color (HP indicator) so we can draw player inside world space
        float hpPct_draw = Clamp((float)player.hp / (float)player.maxHp, 0.0f, 1.0f);
        Color playerColor_draw = {(unsigned char)(hpPct_draw * 255.0f), 0, 0, 255};

        BeginDrawing();
        ClearBackground(BLACK);

        // --- World space (camera transform applied) ---
        BeginMode2D(camera);

            tileMap.draw(visibleWorld);

            if (makerMode && !tileMaker.showMenu)
                tileMaker.drawPreview(worldMouse);

            // Survival mode: show tile cursor
            if (!makerMode && !showInventory)
            {
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
                    
                    // Show one cursor for both mining and placing: selected block color, or white when nothing is selected
                    bool isSolid = tileMap.isSolid(hc, hr);
                    Color cursorColor = selectedInventoryBlockColor(inventory);
                    DrawRectangleLinesEx(hover, 2, cursorColor);

                    // If player is mining, overlay progress directly on the block like Minecraft
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isSolid)
                    {
                        float progress = Clamp(miningHeldTime / MINING_TIME_REQUIRED, 0.0f, 1.0f);
                        // Semi-transparent white overlay scaled by progress
                        DrawRectangle((int)hover.x, (int)hover.y, (int)hover.width, (int)hover.height, Fade(WHITE, 0.2f + 0.6f * progress));
                        // Draw simple crack lines whose alpha increases with progress
                        Color crack = Fade(BLACK, progress);
                        DrawLine((int)hover.x, (int)hover.y, (int)(hover.x + hover.width), (int)(hover.y + hover.height), crack);
                        DrawLine((int)(hover.x + hover.width), (int)hover.y, (int)hover.x, (int)(hover.y + hover.height), crack);
                    }
                }
            }

            // Draw player in world space with HP-indicated color
            DrawCircleV(player.pos, player.size, playerColor_draw);

        EndMode2D();

        // --- Screen space (HUD - no camera transform) ---
        DrawFPS(scrWidth - 80, scrHeight - 20);

        // Zoom indicator
        char zoomText[32];
        snprintf(zoomText, sizeof(zoomText), "ZOOM %.1fx", camera.zoom);
        DrawText(zoomText, scrWidth - 100, scrHeight - 40, 16, DARKGRAY);

        drawConsoleMessages(consoleMessages, fpsMax);

        // === SURVIVAL MODE HUD ===
        if (!makerMode)
        {
            // Player color now indicates HP: interpolate between BLACK (0) and RED (max)
            float hpPct = Clamp((float)player.hp / (float)player.maxHp, 0.0f, 1.0f);
            Color playerColor = {(unsigned char)(hpPct * 255.0f), 0, 0, 255};

            // Draw inventory grid or HUD-less overlay modes
            if (showInventory)
            {
                int invW = 420;
                int invH = 420;
                int invX = (scrWidth - invW) / 2;
                int invY = (scrHeight - invH) / 2;

                DrawRectangle(invX, invY, invW, invH, Fade(BLACK, 0.85f));
                DrawRectangleLinesEx({(float)invX, (float)invY, (float)invW, (float)invH}, 3, YELLOW);
                DrawText("INVENTORY", invX + 20, invY + 12, 26, YELLOW);
                DrawText("Click or press 1-9 to select", invX + 20, invY + 44, 13, DARKGRAY);

                // 3x3 grid
                const int cols = 3;
                const int rows = 3;
                const int slotSize = 100;
                const int padding = 16;
                int gridW = cols * slotSize + (cols - 1) * padding;
                int startX = invX + (invW - gridW) / 2;
                int startY = invY + 80;

                Vector2 mousePos = GetMousePosition();
                for (int idx = 0; idx < Inventory::NUM_SLOTS; ++idx)
                {
                    int cx = idx % cols;
                    int cy = idx / cols;
                    int sx = startX + cx * (slotSize + padding);
                    int sy = startY + cy * (slotSize + padding);

                    Rectangle slotRect = {(float)sx, (float)sy, (float)slotSize, (float)slotSize};
                    bool hovered = CheckCollisionPointRec(mousePos, slotRect);
                    bool selected = (idx == inventory.selectedSlot);

                    Color bg = selected ? Fade(ORANGE, 0.6f) : (hovered ? Fade(WHITE, 0.06f) : Fade(DARKGRAY, 0.2f));
                    Color border = selected ? YELLOW : (hovered ? WHITE : GRAY);

                    DrawRectangleRec(slotRect, bg);
                    DrawRectangleLinesEx(slotRect, selected ? 3 : 2, border);

                    // Slot content preview
                    Color blockColor = DARKGRAY;
                    if (idx == 0) { blockColor = BROWN; }
                    else if (idx == 1) { blockColor = GRAY; }

                    int previewPad = 10;
                    DrawRectangle(sx + previewPad, sy + previewPad, slotSize - previewPad*2, slotSize - previewPad*2 - 20, blockColor);
                    DrawText(TextFormat("x%d", inventory.getBlockCount(idx)), sx + 6, sy + slotSize - 18, 14, YELLOW);

                    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) inventory.selectedSlot = idx;
                }

                DrawText("TAB to close", invX + 20, invY + invH - 25, 12, DARKGRAY);
            }
            else
            {
                // Minimal HUD: show help text only
                DrawText("TAB: Inventory | LMB: Mine | RMB: Place | 1-9: Select Slot", 20, scrHeight - 30, 14, DARKGRAY);
            }

            // Draw the player circle with HP-indicative color (overrides stored player.clr)
            DrawCircleV(player.pos, player.size, playerColor);
        }
        
        // === MAKER MODE HUD ===
        if (makerMode)
        {
            DrawText("MAKER MODE - Press B for block menu", 20, 20, 20, YELLOW);
            DrawText(TextFormat("Block: %s", tileMaker.blockType == 0 ? "Dirt" : tileMaker.blockType == 1 ? "Stone" : "Erase"), 
                     20, 50, 16, WHITE);
            DrawText("L/Drag: Place | R/Drag: Erase | B: Menu | CTRL-M: Survival", 20, scrHeight - 30, 14, DARKGRAY);
        }

        if (pauseMode != NONE)
        {
            DrawRectangle(0, 0, scrWidth, scrHeight, Fade(BLACK, 0.5f));

            const char* title = (pauseMode == SAVING) ? "SAVE LEVEL" : "LOAD LEVEL";
            int titleWidth = MeasureText(title, 60);
            DrawText(title, scrWidth/2 - titleWidth/2, scrHeight/2 - 150, 60, WHITE);

            const char* prompt = "SELECT SLOT (1-5) OR ESC TO CANCEL";
            int promptWidth = MeasureText(prompt, 25);
            DrawText(prompt, scrWidth/2 - promptWidth/2, scrHeight/2 - 50, 25, YELLOW);

            for (int i = 1; i <= MAX_SLOTS; i++)
            {
                int boxX = scrWidth/2 - 300 + (i-1) * 120;
                int boxY = scrHeight/2 + 50;
                
                Rectangle slotRect = {(float)boxX, (float)boxY, 100.0f, 100.0f};
                Vector2 mousePos = GetMousePosition();
                bool isHovered = CheckCollisionPointRec(mousePos, slotRect);

                Color boxColor = slotHasData[i] ? GREEN : GRAY;
                DrawRectangle(boxX, boxY, 100, 100, Fade(boxColor, isHovered ? 0.5f : 0.3f));
                DrawRectangleLinesEx(slotRect, isHovered ? 4 : 3, isHovered ? YELLOW : boxColor);
                
                // Click to select slot
                if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    currentSlot = i;
                    if (pauseMode == SAVING)
                    {
                        saveLevelToSlot(tileMap, player, currentSlot);
                        consoleMessages.push_back({1.5f, "SAVED TO SLOT " + std::to_string(currentSlot), GREEN});
                        pauseMode = NONE;
                    }
                    else if (pauseMode == LOADING)
                    {
                        if (loadLevelFromSlot(tileMap, player, currentSlot))
                            consoleMessages.push_back({1.5f, "LOADED FROM SLOT " + std::to_string(currentSlot), GREEN});
                        else
                            consoleMessages.push_back({1.5f, "SLOT " + std::to_string(currentSlot) + " EMPTY", RED});
                        pauseMode = NONE;
                    }
                }

                std::string slotNum = std::to_string(i);
                int numWidth = MeasureText(slotNum.c_str(), 40);
                DrawText(slotNum.c_str(), boxX + 50 - numWidth/2, boxY + 30, 40, WHITE);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

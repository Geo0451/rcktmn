#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <raylib.h>
#include "rckheaders.cpp"

// ===== CONFIGURATION =====
const int scrWidth  = 1920;
const int scrHeight = 1080;
const int fpsMax    = 60;
const int MAX_SLOTS = 5;

// Camera zoom limits
// 1.0 = zoomed out (current full view), 3.0 = zoomed in (3x)
const float ZOOM_MIN  = 1.0f;
const float ZOOM_MAX  = 3.0f;
const float ZOOM_STEP = 0.15f;

// Player movement constants
const float HORIZONTAL_MOVE_SPEED = 0.85f;
const float VERTICAL_MOVE_SPEED   = 0.8f;
const float JUMP_MULTIPLIER       = 4.0f;
const float GRAVITY               = 0.1f;

// World dimensions in pixels
const int worldWidth  = WORLD_COLS * TILE_SIZE;
const int worldHeight = WORLD_ROWS * TILE_SIZE;

// ===== HELPER FUNCTIONS =====

void handlePlayerInput(Player& player)
{
    if (IsKeyDown(KEY_A)) player.dir.x -= HORIZONTAL_MOVE_SPEED;
    if (IsKeyDown(KEY_D)) player.dir.x += HORIZONTAL_MOVE_SPEED;
    if (IsKeyDown(KEY_S)) player.dir.y += VERTICAL_MOVE_SPEED;

    // Hold space for auto-jump when grounded
    if (IsKeyDown(KEY_SPACE) && player.canJump)
        player.dir.y -= VERTICAL_MOVE_SPEED * JUMP_MULTIPLIER;

    // Single impulse jump fallback
    if (IsKeyPressed(KEY_W) && player.canJump)
        player.dir.y -= VERTICAL_MOVE_SPEED * JUMP_MULTIPLIER;
}

void saveLevelToSlot(const TileMap& tileMap, const Player& player, int slot)
{
    std::string filename = "sf_" + std::to_string(slot) + ".dat";
    std::ofstream saveFile(filename);
    if (!saveFile.is_open()) return;

    saveFile << player.pos.x << " " << player.pos.y << "\n";

    for (int r = 0; r < WORLD_ROWS; r++)
        for (int c = 0; c < WORLD_COLS; c++)
            if (tileMap.isSolid(c, r))
                saveFile << c << " " << r << "\n";

    saveFile.close();
}

bool loadLevelFromSlot(TileMap& tileMap, Player& player, int slot)
{
    std::string filename = "sf_" + std::to_string(slot) + ".dat";
    std::ifstream loadFile(filename);
    if (!loadFile.is_open()) return false;

    if (!(loadFile >> player.pos.x >> player.pos.y))
    {
        loadFile.close();
        return false;
    }

    player.dir    = {0, 0};
    player.canJump = false;
    tileMap.clear();

    int col, row;
    while (loadFile >> col >> row)
        if (tileMap.inBounds(col, row))
            tileMap.tileAt(col, row) = {true, WHITE};

    loadFile.close();
    return true;
}

// O(1) tile-based collision - only checks the tiles the bounding box overlaps
void handleCollisions(Player& player, const Vector2& nextPosition, const TileMap& tileMap)
{
    player.pos    = nextPosition;
    player.canJump = false;

    int left   = (int)(player.pos.x - player.size) / TILE_SIZE;
    int right  = (int)(player.pos.x + player.size) / TILE_SIZE;
    int top    = (int)(player.pos.y - player.size) / TILE_SIZE;
    int bottom = (int)(player.pos.y + player.size) / TILE_SIZE;

    for (int row = top; row <= bottom; row++)
    {
        for (int col = left; col <= right; col++)
        {
            if (!tileMap.isSolid(col, row)) continue;

            Rectangle playerBB = {
                player.pos.x - player.size,
                player.pos.y - player.size,
                player.size * 2,
                player.size * 2
            };

            Rectangle tileRect = tileMap.getTileRect(col, row);
            if (!CheckCollisionRecs(playerBB, tileRect)) continue;

            Rectangle overlap = GetCollisionRec(playerBB, tileRect);

            if (overlap.width > overlap.height) // Floor / ceiling
            {
                player.dir.y = 0;
                float tileMidY = tileRect.y + tileRect.height * 0.5f;
                if (player.pos.y < tileMidY)
                {
                    player.pos.y -= overlap.height;
                    player.canJump = true;
                }
                else
                {
                    player.pos.y += overlap.height;
                }
            }
            else // Left / right wall
            {
                player.dir.x = 0;
                float tileMidX = tileRect.x + tileRect.width * 0.5f;
                if (player.pos.x < tileMidX)
                    player.pos.x -= overlap.width;
                else
                    player.pos.x += overlap.width;
            }
        }
    }
}

void drawConsoleMessages(std::vector<Message>& messages)
{
    if (messages.empty()) return;

    for (int i = 0; i < (int)messages.size(); i++)
    {
        Message* msg = &messages[i];
        DrawText(msg->content.c_str(), 10, 10 + (i * 30), 20, msg->color);

        msg->lifetime -= 1.0f / fpsMax;
        if (msg->lifetime < 0)
        {
            messages.erase(messages.begin() + i);
            i--;
        }
    }
}

// ===== MAIN GAME LOOP =====
int main()
{
    srand(5894);

    Make tileMaker;
    bool makerMode = true;

    int currentSlot = 1;

    enum SaveLoadMode { NONE, SAVING, LOADING };
    enum GameState { MENU, PREVIEW, PLAYING };
    SaveLoadMode pauseMode = NONE;
    GameState gameState = MENU;

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
    bool previewGenerated = false;

    // World generation parameters
    float surfaceHeight = 0.25f;        // 0.0-1.0, where on screen is surface
    float surfaceRoughness = 1.0f;      // 1.0-5.0, higher = more jagged
    float caveDensity = 1.0f;           // 0.1-5.0, how many caves (0.1=rare, 5.0=very dense)
    float caveThreshold = 0.40f;        // 0.05-0.80, higher = fewer caves
    float caveSurfaceBias = 0.22f;      // 0.00-0.50, higher = more surface overlap
    int caveSmoothIterations = 5;       // 1-12, more = smoother/connected caves
    int numWorms = 20;                  // 5-40, number of cave worms (DEPRECATED - kept for compatibility)
    int wormMinLength = 50;             // 30-100 (DEPRECATED)
    int wormMaxLength = 200;            // 150-400 (DEPRECATED)
    int wormMinRadius = 2;              // 1-4 (DEPRECATED)
    int wormMaxRadius = 5;              // 3-7 (DEPRECATED)
    float wormDownwardBias = 0.7f;      // 0.3-1.0 (DEPRECATED)

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
                previewGenerated = true;
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
                previewGenerated = false;
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
                previewGenerated = true;
            }
            
            // Also allow R key to regenerate
            if (IsKeyPressed(KEY_R))
            {
                tileMap.generate(seedVal, surfaceHeight, surfaceRoughness, caveDensity, caveThreshold, caveSurfaceBias, caveSmoothIterations);
                previewGenerated = true;
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
        if (wheel != 0.0f)
        {
            camera.zoom += wheel * ZOOM_STEP;
            float makerZoomMin = (float)scrWidth / (float)worldWidth;
            float makerZoomMinY = (float)scrHeight / (float)worldHeight;
            makerZoomMin = (makerZoomMin < makerZoomMinY) ? makerZoomMin : makerZoomMinY;
            float zoomMin = makerMode ? makerZoomMin : ZOOM_MIN;
            camera.zoom  = Clamp(camera.zoom, zoomMin, ZOOM_MAX);
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
            handlePlayerInput(player);

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
                consoleMessages.push_back(makerMode
                    ? Message{2.0f, "MAKER MODE ON  B:BLOCKS  L:DRAG  R:ERASE  CTRL-M:OFF", WHITE}
                    : Message{1.0f, "MAKER MODE OFF", WHITE});
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
        if (pauseMode == NONE)
        {
            player.dir.y += GRAVITY;
            Vector2 nextPos = player.updated_pos(deltaTime);
            handleCollisions(player, nextPos, tileMap);
        }

        // Camera follows the player in normal play; maker mode can zoom all the way out to a full-map overview
        if (makerMode)
        {
            float makerZoomMin = (float)scrWidth / (float)worldWidth;
            float makerZoomMinY = (float)scrHeight / (float)worldHeight;
            makerZoomMin = (makerZoomMin < makerZoomMinY) ? makerZoomMin : makerZoomMinY;
            if (camera.zoom <= makerZoomMin + 0.0001f)
                camera.target = {(float)worldWidth * 0.5f, (float)worldHeight * 0.5f};
            else
                camera.target = player.pos;
        }
        else
        {
            camera.target = player.pos;
        }

        // ----- RENDERING -----
        BeginDrawing();
        ClearBackground(BLACK);

        // --- World space (camera transform applied) ---
        BeginMode2D(camera);

            tileMap.draw(visibleWorld);

            if (makerMode && !tileMaker.showMenu)
                tileMaker.drawPreview(worldMouse);

            DrawCircleV(player.pos, player.size, player.clr);

        EndMode2D();

        // --- Screen space (HUD - no camera transform) ---
        DrawFPS(scrWidth - 80, scrHeight - 20);

        // Zoom indicator
        char zoomText[32];
        snprintf(zoomText, sizeof(zoomText), "ZOOM %.1fx", camera.zoom);
        DrawText(zoomText, scrWidth - 100, scrHeight - 40, 16, DARKGRAY);

        drawConsoleMessages(consoleMessages);

        if (makerMode && tileMaker.showMenu)
            tileMaker.drawMenu();

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

                Color boxColor = slotHasData[i] ? GREEN : GRAY;
                DrawRectangle(boxX, boxY, 100, 100, Fade(boxColor, 0.3f));
                DrawRectangleLinesEx({(float)boxX, (float)boxY, 100.0f, 100.0f}, 3, boxColor);

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

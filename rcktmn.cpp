#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
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

    // Single impulse jump
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
            if (tileMap.grid[r][c].solid)
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
            tileMap.grid[row][col] = {true, WHITE};

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
    SaveLoadMode pauseMode = NONE;

    bool slotHasData[MAX_SLOTS + 1] = {};

    std::vector<Message> consoleMessages;
    consoleMessages.push_back({
        7.0f,
        "WELCOME 2 RCKTMN\n WASD:MOVE  SCROLL:ZOOM\n CTRL-M:MAKER  L:DRAW  R:ERASE\n CTRL-S/L:SAVE/LOAD"
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

    // ----- Camera setup -----
    Camera2D camera    = {};
    camera.offset      = {scrWidth / 2.0f, scrHeight / 2.0f};
    camera.target      = player.pos;
    camera.rotation    = 0.0f;
    camera.zoom        = 1.0f;

    InitWindow(scrWidth, scrHeight, "RCKTMN");
    SetTargetFPS(fpsMax);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // ----- CAMERA ZOOM (scroll wheel) -----
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
        {
            camera.zoom += wheel * ZOOM_STEP;
            camera.zoom  = Clamp(camera.zoom, ZOOM_MIN, ZOOM_MAX);
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
                    ? Message{2.0f, "MAKER MODE ON  L:DRAW  R:ERASE", WHITE}
                    : Message{1.0f, "MAKER MODE OFF", WHITE});
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
            player.border_check(worldWidth, worldHeight);
        }

        // Camera always follows the player
        camera.target = player.pos;

        // ----- RENDERING -----
        BeginDrawing();
        ClearBackground(BLACK);

        // --- World space (camera transform applied) ---
        BeginMode2D(camera);

            tileMap.draw(visibleWorld);

            if (makerMode)
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

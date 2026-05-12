#include "maker.h"
#include "tilemap.h"
#include "constants.h"
#include <cmath>

Rectangle Make::getDragRect() const
{
    float x = (point_a.x < point_b.x) ? point_a.x : point_b.x;
    float y = (point_a.y < point_b.y) ? point_a.y : point_b.y;
    float w = std::fabs(point_b.x - point_a.x);
    float h = std::fabs(point_b.y - point_a.y);
    return {x, y, w, h};
}

Rectangle Make::getSnappedRect() const
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

void Make::select(TileMap& tileMap, Vector2 worldMouse)
{
    if (IsKeyPressed(KEY_B))
        showMenu = !showMenu;

    if (showMenu)
    {
        if (IsKeyPressed(KEY_ONE)) { blockType = 0; showMenu = false; }
        if (IsKeyPressed(KEY_TWO)) { blockType = 1; showMenu = false; }
        if (IsKeyPressed(KEY_THREE)) { blockType = 2; showMenu = false; }
        if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_ESCAPE)) showMenu = false;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
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
        
        if (blockType == 2)
        {
            tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                  (int)drag.width, (int)drag.height,
                                  false, WHITE);
        }
        else if (blockType == 0)
        {
            tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                  (int)drag.width, (int)drag.height,
                                  true, BROWN);
        }
        else if (blockType == 1)
        {
            tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                  (int)drag.width, (int)drag.height,
                                  true, GRAY);
        }
        dragging = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        point_a  = worldMouse;
        point_b  = worldMouse;
        dragging = true;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && dragging)
        point_b = worldMouse;

    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && dragging)
    {
        point_b = worldMouse;
        Rectangle drag = getDragRect();
        tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                              (int)drag.width, (int)drag.height,
                              false, WHITE);
        dragging = false;
    }
}

void Make::drawPreview(Vector2 worldMouse) const
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
        DrawRectangleLinesEx(hover, 1, Fade(GREEN, 0.5f));
    }

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

void Make::drawMenu() const
{
    int scrW = SCREEN_WIDTH, scrH = SCREEN_HEIGHT;
    int menuW = 450, menuH = 280;
    int x = (scrW - menuW) / 2;
    int y = (scrH - menuH) / 2;
    
    Vector2 mousePos = GetMousePosition();

    DrawRectangle(x, y, menuW, menuH, Fade(BLACK, 0.85f));
    DrawRectangleLinesEx({(float)x, (float)y, (float)menuW, (float)menuH}, 3, YELLOW);

    DrawText("BLOCK TYPE SELECTION", x + 20, y + 15, 22, YELLOW);
    DrawText("Click or press 1-3, B to close", x + 20, y + 50, 14, DARKGRAY);
    
    int optY = y + 85;
    const int optH = 50;
    const int optSpacing = 60;
    
    Rectangle opt1 = {(float)(x + 20), (float)optY, (float)(menuW - 40), (float)optH};
    bool hov1 = CheckCollisionPointRec(mousePos, opt1);
    if (hov1) DrawRectangle(x + 20, optY, menuW - 40, optH, Fade(BROWN, 0.3f));
    DrawRectangleLinesEx(opt1, hov1 ? 2 : 1, (blockType == 0) ? YELLOW : (hov1 ? WHITE : GRAY));
    if (hov1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ((Make*)this)->blockType = 0;
    
    Color c1 = (blockType == 0) ? YELLOW : WHITE;
    DrawText("1. DIRT", x + 40, optY + 12, 18, c1);
    DrawRectangle(x + menuW - 70, optY + 10, 40, 30, BROWN);
    DrawRectangleLinesEx({(float)(x + menuW - 70), (float)(optY + 10), 40.0f, 30.0f}, 1, WHITE);
    
    optY += optSpacing;
    
    Rectangle opt2 = {(float)(x + 20), (float)optY, (float)(menuW - 40), (float)optH};
    bool hov2 = CheckCollisionPointRec(mousePos, opt2);
    if (hov2) DrawRectangle(x + 20, optY, menuW - 40, optH, Fade(GRAY, 0.3f));
    DrawRectangleLinesEx(opt2, hov2 ? 2 : 1, (blockType == 1) ? YELLOW : (hov2 ? WHITE : GRAY));
    if (hov2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ((Make*)this)->blockType = 1;
    
    Color c2 = (blockType == 1) ? YELLOW : WHITE;
    DrawText("2. STONE", x + 40, optY + 12, 18, c2);
    DrawRectangle(x + menuW - 70, optY + 10, 40, 30, GRAY);
    DrawRectangleLinesEx({(float)(x + menuW - 70), (float)(optY + 10), 40.0f, 30.0f}, 1, WHITE);
    
    optY += optSpacing;
    
    Rectangle opt3 = {(float)(x + 20), (float)optY, (float)(menuW - 40), (float)optH};
    bool hov3 = CheckCollisionPointRec(mousePos, opt3);
    if (hov3) DrawRectangle(x + 20, optY, menuW - 40, optH, Fade(RED, 0.2f));
    DrawRectangleLinesEx(opt3, hov3 ? 2 : 1, (blockType == 2) ? YELLOW : (hov3 ? WHITE : GRAY));
    if (hov3 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ((Make*)this)->blockType = 2;
    
    Color c3 = (blockType == 2) ? YELLOW : WHITE;
    DrawText("3. ERASE", x + 40, optY + 12, 18, c3);
    DrawRectangle(x + menuW - 70, optY + 10, 40, 30, BLACK);
    DrawRectangleLinesEx({(float)(x + menuW - 70), (float)(optY + 10), 40.0f, 30.0f}, 2, RED);
}

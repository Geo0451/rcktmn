#pragma once
#include <raylib.h>
#include "constants.h"

// ===== MAKER MODE CLASS =====

class Make
{
public:
    Vector2 point_a = {0, 0};
    Vector2 point_b = {0, 0};
    bool dragging   = false;
    
    int blockType = 0;  // 0=dirt, 1=stone, 2=erase
    bool showMenu = false;

    Rectangle getDragRect() const;
    Rectangle getSnappedRect() const;

    void select(class TileMap& tileMap, Vector2 worldMouse);
    void drawPreview(Vector2 worldMouse) const;
    void drawMenu() const;
};

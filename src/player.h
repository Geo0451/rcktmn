#pragma once
#include <raylib.h>
#include "constants.h"

// ===== PLAYER CLASS =====

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
    
    // Survival mode stats
    int hp = 100;
    int maxHp = 100;
    
    // Maker mode flight
    bool flying = false;
    float flyVelX = 0.0f;
    float flyVelY = 0.0f;

    Vector2 updated_pos(float dt);
};

// ===== INVENTORY CLASS =====

class Inventory
{
public:
    static const int NUM_SLOTS = 9;
    
    int blockCounts[NUM_SLOTS] = {0};
    int selectedSlot = 0;
    
    Inventory();
    void addBlock(int blockType, int count = 1);
    bool removeBlock(int blockType, int count = 1);
    int getBlockCount(int blockType);
};

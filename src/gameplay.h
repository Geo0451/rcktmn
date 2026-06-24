#pragma once
#include <raylib.h>
#include "constants.h"
#include "tilemap.h"

// ===== PHYSICS & COLLISION =====

void tickPlayerPhysics(class Player& player, const TileMap& tileMap, float dt);

// ===== PLAYER INPUT =====

void handlePlayerInput(class Player& player, float dt);

// ===== SAVE/LOAD SYSTEM =====

void saveLevelToSlot(const TileMap& tileMap, const class Player& player, int slot);
bool loadLevelFromSlot(TileMap& tileMap, class Player& player, int slot);

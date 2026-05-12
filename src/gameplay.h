#pragma once
#include <raylib.h>
#include "constants.h"
#include "tilemap.h"

// ===== COLLISION SYSTEM =====

void handleCollisions(class Player& player, const Vector2& nextPosition, const TileMap& tileMap);

// ===== PLAYER INPUT =====

void handlePlayerInput(class Player& player);

// ===== SAVE/LOAD SYSTEM =====

void saveLevelToSlot(const TileMap& tileMap, const class Player& player, int slot);
bool loadLevelFromSlot(TileMap& tileMap, class Player& player, int slot);

#pragma once

// ===== WINDOW & DISPLAY =====
const int SCREEN_WIDTH  = 1920;
const int SCREEN_HEIGHT = 1080;
const int FPS_MAX       = 60;

// ===== WORLD & TILES =====
const int TILE_SIZE   = 20;    // Pixel size per tile
const int WORLD_COLS  = 2048;  // World width in tiles (40,960px)
const int WORLD_ROWS  = 2048;  // World height in tiles (40,960px)

// ===== GAMEPLAY CONSTANTS =====
const int MAX_SAVE_SLOTS = 5;

// Player movement
const float HORIZONTAL_MOVE_SPEED = 0.85f;
const float VERTICAL_MOVE_SPEED   = 0.8f;
const float JUMP_MULTIPLIER       = 4.0f;
const float GRAVITY               = 0.1f;

// Mining
const float MINING_TIME_REQUIRED = 0.3f;

// Camera zoom
const float ZOOM_MIN  = 0.048f;   // See entire map
const float ZOOM_MAX  = 3.0f;     // Zoomed in 3x
const float ZOOM_STEP = 0.08f;    // Smooth increments

// World generation
const float SURFACE_HEIGHT_DEFAULT    = 0.25f;
const float SURFACE_ROUGHNESS_DEFAULT = 1.0f;
const float CAVE_DENSITY_DEFAULT      = 1.0f;
const float CAVE_THRESHOLD_DEFAULT    = 0.40f;
const float CAVE_SURFACE_BIAS_DEFAULT = 0.22f;
const int CAVE_SMOOTH_ITERATIONS_DEFAULT = 5;

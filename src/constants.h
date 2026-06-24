#pragma once

// ===== WINDOW & DISPLAY =====
const int SCREEN_WIDTH  = 1920;
const int SCREEN_HEIGHT = 1080;
const int FPS_MAX       = 60;

// ===== WORLD & TILES =====
const int TILE_SIZE   = 20;    // Pixel size per tile
// For web demo builds we use a much smaller world to fit Wasm memory.
#ifdef WEB_BUILD
#define WORLD_COLS 1024
#define WORLD_ROWS 1024
#else
#define WORLD_COLS 2048
#define WORLD_ROWS 2048
#endif

// ===== GAMEPLAY CONSTANTS =====
const int MAX_SAVE_SLOTS = 5;

// Player movement (tuned for 60 Hz reference; scaled by dt in physics tick)
const float HORIZONTAL_MOVE_SPEED = 0.85f;
const float VERTICAL_MOVE_SPEED   = 0.8f;
const float JUMP_MULTIPLIER       = 4.0f;
const float GRAVITY               = 0.1f;
const float PHYSICS_FPS_REF       = 60.0f;
const float COYOTE_TIME           = 0.12f;
const float JUMP_BUFFER_TIME      = 0.12f;

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

#include "gameplay.h"
#include "player.h"
#include "tilemap.h"
#include "constants.h"
#include <algorithm>
#include <cmath>
#include <fstream>

template<typename T>
static T ClampVal(T value, T minVal, T maxVal)
{
    return std::max(minVal, std::min(value, maxVal));
}

static Rectangle playerBounds(const Player& player)
{
    return {
        player.pos.x - player.size,
        player.pos.y - player.size,
        player.size * 2.0f,
        player.size * 2.0f
    };
}

static void tileRangeForPlayer(const Player& player, int& left, int& right, int& top, int& bottom)
{
    left   = (int)std::floor((player.pos.x - player.size) / TILE_SIZE);
    right  = (int)std::floor((player.pos.x + player.size) / TILE_SIZE);
    top    = (int)std::floor((player.pos.y - player.size) / TILE_SIZE);
    bottom = (int)std::floor((player.pos.y + player.size) / TILE_SIZE);
}

static void resolveMoveX(Player& player, const TileMap& tileMap, float dx)
{
    if (dx == 0.0f) return;

    player.pos.x += dx;

    int left, right, top, bottom;
    tileRangeForPlayer(player, left, right, top, bottom);

    for (int row = top; row <= bottom; row++)
    {
        for (int col = left; col <= right; col++)
        {
            if (!tileMap.isSolid(col, row)) continue;

            Rectangle tileRect = tileMap.getTileRect(col, row);
            Rectangle bb = playerBounds(player);
            if (!CheckCollisionRecs(bb, tileRect)) continue;

            if (dx > 0.0f)
            {
                player.pos.x = tileRect.x - player.size;
                if (player.flying) player.flyVelX = 0.0f;
                else player.dir.x = 0.0f;
            }
            else
            {
                player.pos.x = tileRect.x + tileRect.width + player.size;
                if (player.flying) player.flyVelX = 0.0f;
                else player.dir.x = 0.0f;
            }
        }
    }
}

static void resolveMoveY(Player& player, const TileMap& tileMap, float dy)
{
    if (dy == 0.0f) return;

    player.pos.y += dy;

    int left, right, top, bottom;
    tileRangeForPlayer(player, left, right, top, bottom);

    for (int row = top; row <= bottom; row++)
    {
        for (int col = left; col <= right; col++)
        {
            if (!tileMap.isSolid(col, row)) continue;

            Rectangle tileRect = tileMap.getTileRect(col, row);
            Rectangle bb = playerBounds(player);
            if (!CheckCollisionRecs(bb, tileRect)) continue;

            if (dy > 0.0f)
            {
                player.pos.y = tileRect.y - player.size;
                if (player.flying) player.flyVelY = 0.0f;
                else
                {
                    player.dir.y = 0.0f;
                    player.onGround = true;
                }
            }
            else
            {
                player.pos.y = tileRect.y + tileRect.height + player.size;
                if (player.flying) player.flyVelY = 0.0f;
                else player.dir.y = 0.0f;
            }
        }
    }
}

static void moveAxisX(Player& player, const TileMap& tileMap, float totalDx)
{
    const float maxStep = TILE_SIZE * 0.5f;
    float remaining = totalDx;

    while (std::fabs(remaining) > 0.0001f)
    {
        float step = remaining;
        if (std::fabs(step) > maxStep)
            step = (remaining > 0.0f) ? maxStep : -maxStep;

        remaining -= step;
        resolveMoveX(player, tileMap, step);
    }
}

static void moveAxisY(Player& player, const TileMap& tileMap, float totalDy)
{
    if (totalDy == 0.0f) return;

    player.onGround = false;

    const float maxStep = TILE_SIZE * 0.5f;
    float remaining = totalDy;

    while (std::fabs(remaining) > 0.0001f)
    {
        float step = remaining;
        if (std::fabs(step) > maxStep)
            step = (remaining > 0.0f) ? maxStep : -maxStep;

        remaining -= step;
        resolveMoveY(player, tileMap, step);
    }
}

void handlePlayerInput(Player& player, float dt)
{
    const float frameScale = dt * PHYSICS_FPS_REF;

    if (IsKeyDown(KEY_A)) player.dir.x -= HORIZONTAL_MOVE_SPEED * frameScale;
    if (IsKeyDown(KEY_D)) player.dir.x += HORIZONTAL_MOVE_SPEED * frameScale;
    if (IsKeyDown(KEY_S)) player.dir.y += VERTICAL_MOVE_SPEED * frameScale;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W))
        player.jumpBufferTimer = JUMP_BUFFER_TIME;
}

void tickPlayerPhysics(Player& player, const TileMap& tileMap, float dt)
{
    const float frameScale = dt * PHYSICS_FPS_REF;

    if (player.jumpBufferTimer > 0.0f)
        player.jumpBufferTimer -= dt;

    if (!player.flying &&
        player.jumpBufferTimer > 0.0f &&
        player.coyoteTimer > 0.0f)
    {
        player.dir.y = -VERTICAL_MOVE_SPEED * JUMP_MULTIPLIER;
        player.jumpBufferTimer = 0.0f;
        player.coyoteTimer = 0.0f;
        player.onGround = false;
    }

    if (!player.flying)
        player.dir.y += GRAVITY * frameScale;

    player.dir.x = ClampVal(player.dir.x, -player.x_max_vel, player.x_max_vel);

    if (player.flying)
    {
        player.flyVelX = ClampVal(player.flyVelX, -player.x_max_vel * 2.0f, player.x_max_vel * 2.0f);
        player.flyVelY = ClampVal(player.flyVelY, -player.y_max_vel * 2.0f, player.y_max_vel * 2.0f);

        if (player.flyVelX > -0.1f && player.flyVelX < 0.1f)
            player.flyVelX = 0.0f;
        else
            player.flyVelX *= std::pow(0.95f, frameScale);

        if (player.flyVelY > -0.1f && player.flyVelY < 0.1f)
            player.flyVelY = 0.0f;
        else
            player.flyVelY *= std::pow(0.95f, frameScale);
    }
    else
    {
        player.dir.y = ClampVal(player.dir.y, -player.y_max_vel * 1.5f, player.y_max_vel * 2.0f);

        if (player.dir.x > -0.1f && player.dir.x < 0.1f)
            player.dir.x = 0.0f;
        else
            player.dir.x *= std::pow(1.0f / player.x_friction, frameScale);
    }

    const float velX = player.flying ? player.flyVelX : player.dir.x;
    const float velY = player.flying ? player.flyVelY : player.dir.y;
    const float moveX = velX * player.speed * frameScale;
    const float moveY = velY * player.speed * frameScale;

    moveAxisX(player, tileMap, moveX);
    moveAxisY(player, tileMap, moveY);

    if (player.onGround)
        player.coyoteTimer = COYOTE_TIME;
    else
        player.coyoteTimer -= dt;

    player.canJump = !player.flying && player.coyoteTimer > 0.0f;
}

// For web builds we disable persistent save/load (use in-memory only or IndexedDB externally)
#ifdef WEB_BUILD
void saveLevelToSlot(const TileMap& tileMap, const Player& player, int slot)
{
    // No-op in web demo build. Persistence should be handled using IDBFS mounts in the HTML build script.
}

bool loadLevelFromSlot(TileMap& tileMap, Player& player, int slot)
{
    // No persistent load available in web demo build.
    return false;
}
#else
void saveLevelToSlot(const TileMap& tileMap, const Player& player, int slot)
{
    std::string filename = "sf_" + std::to_string(slot) + ".dat";
    std::ofstream saveFile(filename);
    if (!saveFile.is_open()) return;

    saveFile << player.pos.x << " " << player.pos.y << "\n";

    for (int r = 0; r < WORLD_ROWS; r++)
        for (int c = 0; c < WORLD_COLS; c++)
            if (tileMap.isSolid(c, r))
            {
                const Tile& t = tileMap.tileAt(c, r);
                saveFile << c << " " << r << " " << (int)t.color.r << " " << (int)t.color.g << " " << (int)t.color.b << " " << (int)t.color.a << "\n";
            }

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

    player.dir = {0, 0};
    player.flyVelX = 0.0f;
    player.flyVelY = 0.0f;
    player.canJump = false;
    player.onGround = false;
    player.coyoteTimer = 0.0f;
    player.jumpBufferTimer = 0.0f;
    tileMap.clear();

    int col, row, r, g, b, a;
    while (loadFile >> col >> row)
    {
        if (!(loadFile >> r >> g >> b >> a))
        {
            r = 255; g = 255; b = 255; a = 255;
        }
        
        if (tileMap.inBounds(col, row))
        {
            Color blockColor = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
            tileMap.tileAt(col, row) = {true, blockColor};
        }
    }

    loadFile.close();
    return true;
}
#endif

#include "gameplay.h"
#include "player.h"
#include "tilemap.h"
#include "constants.h"
#include <fstream>

void handlePlayerInput(Player& player)
{
    if (IsKeyDown(KEY_A)) player.dir.x -= HORIZONTAL_MOVE_SPEED;
    if (IsKeyDown(KEY_D)) player.dir.x += HORIZONTAL_MOVE_SPEED;
    if (IsKeyDown(KEY_S)) player.dir.y += VERTICAL_MOVE_SPEED;

    if (IsKeyDown(KEY_SPACE) && player.canJump)
        player.dir.y -= VERTICAL_MOVE_SPEED * JUMP_MULTIPLIER;

    if (IsKeyPressed(KEY_W) && player.canJump)
        player.dir.y -= VERTICAL_MOVE_SPEED * JUMP_MULTIPLIER;
}

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

            if (overlap.width > overlap.height)
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
            else
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

    player.dir    = {0, 0};
    player.canJump = false;
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

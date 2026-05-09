#include <cmath>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// ===== WORLD GRID CONSTANTS =====
const int TILE_SIZE  = 20;   // Matches player diameter (radius 10 * 2)
const int WORLD_COLS = 400;  // World width  in tiles (8000px)
const int WORLD_ROWS = 112;  // World height in tiles (2240px)

// ===== MESSAGE =====

class Message
{
    public:
        float lifetime = 1.0f;
        std::string content = "Message";
        Color color = WHITE;
};

// ===== TILE MAP =====

struct Tile
{
    bool solid = false;
    Color color = WHITE;
};

class TileMap
{
    public:
        Tile grid[WORLD_ROWS][WORLD_COLS];

        TileMap()
        {
            for (int r = 0; r < WORLD_ROWS; r++)
                for (int c = 0; c < WORLD_COLS; c++)
                    grid[r][c] = {false, WHITE};
        }

        bool inBounds(int col, int row) const
        {
            return col >= 0 && col < WORLD_COLS && row >= 0 && row < WORLD_ROWS;
        }

        bool isSolid(int col, int row) const
        {
            if (!inBounds(col, row)) return false;
            return grid[row][col].solid;
        }

        Rectangle getTileRect(int col, int row) const
        {
            return {
                (float)(col * TILE_SIZE),
                (float)(row * TILE_SIZE),
                (float)TILE_SIZE,
                (float)TILE_SIZE
            };
        }

        // Fill a region defined by world-pixel coordinates with solid tiles
        void fillPixelRect(int px, int py, int pw, int ph, bool solid, Color color = WHITE)
        {
            // Ensure at least one tile is covered (handles single-click px=pw=0)
            if (pw < 1) pw = 1;
            if (ph < 1) ph = 1;

            int c0 =  px              / TILE_SIZE;
            int r0 =  py              / TILE_SIZE;
            int c1 = (px + pw - 1)   / TILE_SIZE;
            int r1 = (py + ph - 1)   / TILE_SIZE;

            for (int r = r0; r <= r1; r++)
                for (int c = c0; c <= c1; c++)
                    if (inBounds(c, r))
                    {
                        grid[r][c].solid = solid;
                        grid[r][c].color = color;
                    }
        }

        void eraseAtPixel(int px, int py)
        {
            int c = px / TILE_SIZE;
            int r = py / TILE_SIZE;
            if (inBounds(c, r)) grid[r][c] = {false, WHITE};
        }

        void clear()
        {
            for (int r = 0; r < WORLD_ROWS; r++)
                for (int c = 0; c < WORLD_COLS; c++)
                    grid[r][c] = {false, WHITE};
        }

        // Only draw tiles in the visible rect (viewport culling)
        void draw(Rectangle visibleWorld) const
        {
            int c0 = (int)(visibleWorld.x)                       / TILE_SIZE - 1;
            int r0 = (int)(visibleWorld.y)                       / TILE_SIZE - 1;
            int c1 = (int)(visibleWorld.x + visibleWorld.width)  / TILE_SIZE + 1;
            int r1 = (int)(visibleWorld.y + visibleWorld.height) / TILE_SIZE + 1;

            c0 = c0 < 0 ? 0 : c0;
            r0 = r0 < 0 ? 0 : r0;
            c1 = c1 >= WORLD_COLS ? WORLD_COLS - 1 : c1;
            r1 = r1 >= WORLD_ROWS ? WORLD_ROWS - 1 : r1;

            for (int r = r0; r <= r1; r++)
                for (int c = c0; c <= c1; c++)
                    if (grid[r][c].solid)
                        DrawRectangle(
                            c * TILE_SIZE, r * TILE_SIZE,
                            TILE_SIZE, TILE_SIZE,
                            grid[r][c].color
                        );
        }
};

// ===== PLAYER =====

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

        Vector2 updated_pos(float dt)
        {
            dt *= 60;

            dir.x = Clamp(dir.x, -x_max_vel, +x_max_vel);
            dir.y = Clamp(dir.y, -y_max_vel * 1.5f, +y_max_vel * 2.0f);

            if (dir.x < 0.1f && dir.x > -0.1f)
                dir.x = 0;
            else
                dir.x = dir.x / x_friction;

            return {
                pos.x + (dir.x * speed * dt),
                pos.y + (dir.y * speed * dt)
            };
        }

        void border_check(int worldWidth, int worldHeight)
        {
            if (pos.x > worldWidth)  pos.x = (float)worldWidth;
            if (pos.x < 0)           pos.x = 0;
            if (pos.y > worldHeight) pos.y = (float)worldHeight;
            if (pos.y < 0)           pos.y = 0;
        }
};

// ===== MAKER MODE =====

class Make
{
    public:
        Vector2 point_a = {0, 0};
        Vector2 point_b = {0, 0};
        bool dragging   = false;

        // Raw drag rect in world space
        Rectangle getDragRect() const
        {
            float x = (point_a.x < point_b.x) ? point_a.x : point_b.x;
            float y = (point_a.y < point_b.y) ? point_a.y : point_b.y;
            float w = fabsf(point_b.x - point_a.x);
            float h = fabsf(point_b.y - point_a.y);
            return {x, y, w, h};
        }

        // Drag rect snapped to tile grid (visual ghost)
        Rectangle getSnappedRect() const
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

        // worldMouse = GetScreenToWorld2D(GetMousePosition(), camera)
        void select(TileMap& tileMap, Vector2 worldMouse)
        {
            // Left-click: start drag (or single-click to place one tile)
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
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
                // pw/ph of 0 → single tile click, fillPixelRect handles the min-1 guard
                tileMap.fillPixelRect((int)drag.x, (int)drag.y,
                                      (int)drag.width, (int)drag.height,
                                      true, WHITE);
                dragging = false;
            }

            // Right-click: erase tile under cursor
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                tileMap.eraseAtPixel((int)worldMouse.x, (int)worldMouse.y);
        }

        // Call this inside BeginMode2D so coordinates match the world
        void drawPreview(Vector2 worldMouse) const
        {
            // Tile under cursor highlight
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

            // Drag ghost
            if (dragging)
            {
                Rectangle snapped = getSnappedRect();
                DrawRectangle((int)snapped.x, (int)snapped.y,
                              (int)snapped.width, (int)snapped.height,
                              Fade(GREEN, 0.2f));
                DrawRectangleLinesEx(snapped, 1, GREEN);
            }
        }
};

#include "player.h"
#include <algorithm>

template<typename T>
T Clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(value, max_val));
}

Vector2 Player::updated_pos(float dt)
{
    dt *= 60;

    dir.x = Clamp(dir.x, -x_max_vel, +x_max_vel);
    
    if (flying)
    {
        flyVelX = Clamp(flyVelX, -x_max_vel * 2.0f, x_max_vel * 2.0f);
        if (flyVelX < 0.1f && flyVelX > -0.1f)
            flyVelX = 0;
        else
            flyVelX *= 0.95f;

        flyVelY = Clamp(flyVelY, -y_max_vel * 2.0f, y_max_vel * 2.0f);
        if (flyVelY < 0.1f && flyVelY > -0.1f)
            flyVelY = 0;
        else
            flyVelY *= 0.95f;
    }
    else
    {
        dir.y = Clamp(dir.y, -y_max_vel * 1.5f, +y_max_vel * 2.0f);
    }

    if (dir.x < 0.1f && dir.x > -0.1f)
        dir.x = 0;
    else
        dir.x = dir.x / x_friction;

    float newY = pos.y + (flying ? flyVelY * speed * dt : dir.y * speed * dt);
    float newX = pos.x + (flying ? flyVelX * speed * dt : dir.x * speed * dt);
    
    return {
        newX,
        newY
    };
}

Inventory::Inventory()
{
    blockCounts[0] = 64;
    blockCounts[1] = 32;
}

void Inventory::addBlock(int blockType, int count)
{
    if (blockType >= 0 && blockType < NUM_SLOTS)
        blockCounts[blockType] += count;
}

bool Inventory::removeBlock(int blockType, int count)
{
    if (blockType >= 0 && blockType < NUM_SLOTS && blockCounts[blockType] >= count)
    {
        blockCounts[blockType] -= count;
        return true;
    }
    return false;
}

int Inventory::getBlockCount(int blockType)
{
    if (blockType >= 0 && blockType < NUM_SLOTS)
        return blockCounts[blockType];
    return 0;
}

#include "player.h"
#include <algorithm>

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

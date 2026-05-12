#pragma once
#include <raylib.h>
#include "constants.h"

// ===== MESSAGE SYSTEM =====

struct Message
{
    float lifetime = 1.0f;
    std::string content = "Message";
    Color color = WHITE;
};

void drawConsoleMessages(std::vector<Message>& messages, int fpsMax)
{
    if (messages.empty()) return;

    for (int i = 0; i < (int)messages.size(); i++)
    {
        Message* msg = &messages[i];
        DrawText(msg->content.c_str(), 10, 10 + (i * 30), 20, msg->color);

        msg->lifetime -= 1.0f / fpsMax;
        if (msg->lifetime < 0)
        {
            messages.erase(messages.begin() + i);
            i--;
        }
    }
}

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <raylib.h>
#include "rckheaders.cpp"
#include "fstream"



const int scrWidth = 1920;
const int scrHeight = 1080;
const int fpsMax = 60;

float xMSpeed = 0.85f; //movement speed coefficients for WASD
float yMSpeed = 0.8f;

//const int platformsMax = 100;

Vector2 upos;

int main()
{
    srand(5894);

    Make draw;
    draw.npt.color = WHITE;
    bool makerMode = true;
    std::ofstream saveFile("sf.dat");
    std::vector<Message> consoleMessages;
    consoleMessages.push_back({5.0, "WELCOME 2 RCKTMN ALPHA\n - WASD 2 MOVE\n - CTRL-S 2 SAVE\n - CTRL-M 4 TOGGLE MAKERMODE\n - DONT FALL"});

    Player plr = {
        (Vector2) {scrWidth / 4.0f, scrHeight / 4.0f}, // pos
        (Vector2) {0.1, 0}, // dir
        2.0f, // speed
        10.0f, // size
        RED, // color

        1.05f, // x-axis friction
        2.8f, // x max velocity
        3.0f // y max velocity
    };    

    //create some random platforms
    std::vector<Platform> platforms;
 
    InitWindow(scrWidth, scrHeight, "RCKTMN");
    SetTargetFPS(fpsMax);
    
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();   
        //WASD Controls
        if (IsKeyDown(KEY_A)) plr.dir.x -= xMSpeed;
        if (IsKeyDown(KEY_D)) plr.dir.x += xMSpeed;
        //if (IsKeyDown(KEY_W)) plr.dir.y -= yMSpeed;
        if (IsKeyDown(KEY_W)) 
        {
            if (plr.canJump)
            {
                plr.dir.y -= yMSpeed * 4;
                
            } 
        }
        if (IsKeyDown(KEY_S)) plr.dir.y += yMSpeed;
        
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_M))
        {
            makerMode = !makerMode;
            if (makerMode) consoleMessages.push_back({2.0,"MAKER MODE ON L:CREATE R:DESTROY",WHITE});
            else consoleMessages.push_back({1.0,"MAKER MODE OFF",WHITE});
        }
        if (makerMode)
        {
            draw.select(platforms); // call rect creation method, add to refered global platforms vector array 
            DrawRectangleLinesEx(draw.npt.body, 1, GREEN);        
        }

        if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
        {
            for (Platform& p: platforms)
            {
                saveFile << p.body.x << " " << p.body.y << " " << p.body.width << " " << p.body.height << " \n";
            }
            consoleMessages.push_back({1.0,"FILE SAVED",WHITE});

        }
    

        //update everything
        plr.dir.y += 0.1f;
        upos = plr.updated_pos(dt);      
        
        
        // collision code
        // checkrect is a bounding box for the player
        Rectangle checkrect = {upos.x - plr.size,upos.y - plr.size,plr.size*2,plr.size*2};
        bool collided = false;
        int i;
        for (i = 0; i < platforms.size(); ++i)
        {            
            if (CheckCollisionRecs(checkrect, platforms[i].body))
            {
                collided = true;
                break;
            }        
        }
        if (collided) 
        {
            Rectangle overlap = GetCollisionRec(checkrect, platforms[i].body);

                    if (overlap.width > overlap.height) //y axis collision, (top or bottom)
                    {
                        //std::cout << "AAA\n";
                        plr.dir.y = 0;
                        if (plr.pos.y < platforms[i].body.y)  
                        {
                            plr.pos.y -= overlap.height;
                            plr.canJump = true;
                        }
                        else plr.pos.y += overlap.height;
                    }
                    else 
                    {
                        plr.dir.x = 0;
                        if (plr.pos.x < platforms[i].body.x)  plr.pos.x -= overlap.width;
                        else plr.pos.x += overlap.width;

                    }                
        }
        else  
        {
            plr.pos = upos;  // no collision? use updated position as usual       
            plr.canJump = false;  
        }
        plr.border_check(scrWidth, scrHeight);
        
        ClearBackground(BLACK);
        BeginDrawing();
        DrawFPS(10, scrHeight - 20);

        //draw all rectangles
        for (int i = 0; i < platforms.size(); ++i)
        {
            DrawRectangleRec(platforms[i].body, platforms[i].color);
        }
        
        //draw player hitbox(?) and then player
        //DrawRectangleRec({plr.pos.x - plr.size,plr.pos.y - plr.size,plr.size*2,plr.size*2}, GREEN); //bounding box
        DrawCircleV(plr.pos, plr.size, plr.clr);
        
        

        //  console messages
        if (!consoleMessages.empty())
        { 
            int msgY = 10;
            for (int i = 0; i < consoleMessages.size(); i++)
            {
                Message *msg = &consoleMessages[i];
                DrawText(msg->content.c_str(), 10, msgY + (i * 30), 30, msg->color);
                msg->lifetime -= 1.0/fpsMax;
                if (msg->lifetime < 0) consoleMessages.erase(consoleMessages.begin() + i);
                
            }
        }
        
        
        EndDrawing();
    }
    saveFile.close();
    return 0;
}

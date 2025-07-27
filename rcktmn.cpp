#include <cstdlib>
#include <iostream>
#include <raylib.h>
#include "rckheaders.cpp"



const int scrWidth = 1000;
const int scrHeight = 1000;
const int fpsMax = 60;

float xMSpeed = 0.85f; //movement speed coefficients for WASD
float yMSpeed = 0.8f;

const int platformsMax = 10;

Vector2 upos;

int main()
{
    srand(5854);
    Make draw;
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
    Platform platforms[256];
    
    for (int i = 0; i < platformsMax; ++i) 
    {
        platforms[i].body =     (Rectangle) {
             (float) (rand() % 1000), // x
             (float) (rand() % 1000), // y
             (float) (rand() % (100 - 50 + 1) + 50), // width, uses formula rand() - (max - min + 1) + min to clamp random
             (float) (rand() % (100 - 50 + 1) + 50) }; //height
        
        platforms[i].color = WHITE;
        platforms[i].dir = {0,0};
        /*platforms[i].dir = *{
            (float) (rand() % 10) / 2,
            (float) (rand() % 10) / 2 };*/
    }
 
    InitWindow(scrWidth, scrHeight, "pgame dot exe");
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

        draw.select();


        //update everything
        plr.dir.y += 0.1f;
        upos = plr.updated_pos(dt);      
        
        
        // collision code
        // checkrect is a bounding box for the player
        Rectangle checkrect = {upos.x - plr.size,upos.y - plr.size,plr.size*2,plr.size*2};
        bool collided = false;
        int i;
        for (i = 0; i < platformsMax; ++i)
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
        DrawFPS(10, 10);

        //draw all rectangles
        for (int i = 0; i < platformsMax; ++i)
        {
            DrawRectangleRec(platforms[i].body, platforms[i].color);
        }
        
        //draw player hitbox(?) and then player
        DrawRectangleRec({plr.pos.x - plr.size,plr.pos.y - plr.size,plr.size*2,plr.size*2}, GREEN); //bounding box
        DrawCircleV(plr.pos, plr.size, plr.clr);

        //DrawRectangleRec(draw.selection,GREEN);
        DrawRectangleLinesEx(draw.selection, 1, GREEN);

        EndDrawing();
        
        

    }
    return 0;
}

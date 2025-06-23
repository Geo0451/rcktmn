#include <cstdlib>
#include <iostream>
#include <raylib.h>
#include "rckheaders.cpp"



const int scrWidth = 1000;
const int scrHeight = 1000;
const int fpsMax = 60;

float xMSpeed = 0.9f; //movement speed coefficients for WASD
float yMSpeed = 0.8f;

const int platformsMax = 80;

Vector2 upos;

int main()
{
    srand(54);
    Player dude = {
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
             (float) (rand() % 100), // width
             (float) (rand() % 100) }; //height
        
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

        if (IsKeyDown(KEY_A)) dude.dir.x -= xMSpeed;
        if (IsKeyDown(KEY_D)) dude.dir.x += xMSpeed;
        if (IsKeyDown(KEY_W)) dude.dir.y -= yMSpeed;
        if (IsKeyDown(KEY_S)) dude.dir.y += yMSpeed;

        //update everything
        dude.dir.y += 0.1f;
        upos = dude.updated_pos(dt);      
        
        
        // collision code
        // checkrect is a bounding box for the player
        Rectangle checkrect = {upos.x - dude.size,upos.y - dude.size,dude.size*2,dude.size*2};
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
        if (collided == false) dude.pos = upos; // no collision? use updated position as usual
        else 
        {
            Rectangle overlap = GetCollisionRec(checkrect, platforms[i].body);

                    if (overlap.width > overlap.height) //y axis collision, (top or bottom)
                    {
                        //std::cout << "AAA\n";
                        dude.dir.y = 0;
                        if (dude.pos.y < platforms[i].body.y)  dude.pos.y -= overlap.height;
                        else dude.pos.y += overlap.height;
                    }
                    else 
                    {
                        dude.dir.x = 0;
                        if (dude.pos.x < platforms[i].body.x)  dude.pos.x -= overlap.width;
                        else dude.pos.x += overlap.width;

                    }                
        }            
        
        dude.border_check(scrWidth, scrHeight);
        
        //up & down
        /*if (platforms[i]->body.y + platforms[i]->body.height > scrHeight - scrHeight/4.0 || platforms[i]->body.y + platforms[i]->body.height < scrHeight/4.0 ) 
        {
            platforms[i]->dir.y = -platforms[i]->dir.y;
        }
        if (platforms[i]->body.x + platforms[i]->body.width > scrWidth - scrWidth/4.0 || platforms[i]->body.x + platforms[i]->body.width < scrWidth/4.0 ) 
        {
            platforms[i]->dir.x = -platforms[i]->dir.x;
        }*/


        ClearBackground(BLACK);
        BeginDrawing();
        DrawFPS(10, 10);

        //draw all rectangles
        for (int i = 0; i < platformsMax; ++i)
        {
            DrawRectangleRec(platforms[i].body, platforms[i].color);
        }
        
        //draw player hitbox(?) and then player
        //DrawRectangleRec({dude.pos.x - dude.size,dude.pos.y - dude.size,dude.size*2,dude.size*2}, GREEN); //bounding box
        DrawCircleV(dude.pos, dude.size, dude.clr);

        EndDrawing();
        
        

    }
    return 0;
}

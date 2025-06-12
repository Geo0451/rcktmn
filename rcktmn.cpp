#include <iostream>
#include <raylib.h>
#include "rckheaders.cpp"
#include <random>

const int scrWidth = 1000;
const int scrHeight = 1000;
const int fpsMax = 60;

float xMSpeed = 0.9; //movement speed coefficients
float yMSpeed = 0.8;

const int platformsMax = 8;

Vector2 upos;

int main()
{
    Player dude;
    dude.clr = RED;

    dude.pos.x = scrWidth / 4.0;
    dude.pos.y = scrHeight / 4.0;
    dude.dir.x = 0.1;
    dude.dir.y = 0;
    dude.speed = 2;
    dude.size = 10;

    dude.x_friction = 1.05;
    dude.x_max_vel = 2.8;
    dude.y_max_vel = 3.0;
    
    Platform platforms[256];
    //Platform* ppt = platforms[0];
    for (int i = 0; i < platformsMax; ++i) 
    {

       

        platforms[i].color = WHITE;

        platforms[i].body.x = (rand() % 1000);
        platforms[i].body.y = (rand() % 1000);
        platforms[i].body.width = (rand() % 100);
        platforms[i].body.height = (rand() % 100);

        platforms[i].dir = {(float) (rand() % 10) / 2,(float) (rand() % 10) / 2};
    }
 
    InitWindow(scrWidth, scrHeight, "pgame dot exe");
    SetTargetFPS(fpsMax);
    
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();   
        //std::cout << dt;
        if (IsKeyDown(KEY_A)) dude.dir.x -= xMSpeed;
        if (IsKeyDown(KEY_D)) dude.dir.x += xMSpeed;
        if (IsKeyDown(KEY_W)) dude.dir.y -= yMSpeed;
        if (IsKeyDown(KEY_S)) dude.dir.y += yMSpeed;

        //std::cout << dude.dir.x << " " << dude.dir.y << std::endl;
        Rectangle checkrect = {upos.x - dude.size,upos.y - dude.size,dude.size*2,dude.size*2};
        ClearBackground(BLACK);
        BeginDrawing();
        DrawFPS(10, 10);


        for (int i = 0; i < platformsMax; ++i)
        {
            DrawRectangleRec(platforms[i].body, platforms[i].color);
        }
        
        //DrawRectangleRec(checkrect, GREEN); //bounding box
        DrawCircleV(dude.pos, dude.size, dude.clr);

        EndDrawing();

        upos = dude.updated_pos(dt);
        

        dude.platformCollision(dt,&upos,platforms, platformsMax);
            
        
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
        
        

    }
    return 0;
}

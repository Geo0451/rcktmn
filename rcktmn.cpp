#include <iostream>
#include <raylib.h>
#include "rckheaders.cpp"
#include <random>

const int scrWidth = 1000;
const int scrHeight = 1000;
const int fpsMax = 60;

float xMSpeed = 0.9; //movement speed coefficients
float yMSpeed = 0.8;

const int platformMax = 8;

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
    
    Platform* platforms[256];
    //Platform* ppt = platforms[0];
    for (int i = 0; i < platformMax; ++i) 
    {

        platforms[i] = new Platform;
        platforms[i]->body =
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


        for (int i = 0; i < platformMax; ++i)
        {
            DrawRectangleRec(platforms[i]->body, platforms[i]->color);
        }
        
        //DrawRectangleRec(checkrect, GREEN); //bounding box
        DrawCircleV(dude.pos, dude.size, dude.clr);

        EndDrawing();

        upos = dude.updated_pos(dt);
        checkrect = {upos.x - dude.size,upos.y - dude.size,dude.size*2,dude.size*2};

        if (CheckCollisionRecs(checkrect, baseplate.body))
        {
            Rectangle overlap = GetCollisionRec(checkrect, baseplate.body);

            if (overlap.width > overlap.height) //y axis collision
            {
                if (dude.pos.y < baseplate.body.y)
                {
                    dude.pos.x = upos.x;
                    dude.pos.y += baseplate.dir.y;
                    dude.dir.y = 0;
                    //dude.dir.x = baseplate.dir.x;
                }
                if (dude.pos.y > baseplate.body.y + baseplate.body.height)
                {
                    dude.pos.x = upos.x;
                    dude.pos.y += baseplate.dir.y;
                    dude.dir.y = 0;
                }
            }
            else //x axis
            {
                if (dude.pos.x < baseplate.body.x)
                {
                    dude.pos.y = upos.y;
                    dude.pos.x += baseplate.dir.x;
                    dude.dir.x = 0;
                }
                if (dude.pos.x > baseplate.body.x + baseplate.body.width)
                {
                    dude.pos.y = upos.y;
                    dude.pos.x += baseplate.dir.x;
                    dude.dir.x = 0;
                }

            }
            
        }
        else dude.pos = upos;
         
            
            
        
        dude.border_check(scrWidth, scrHeight);
        
        //up & down
        /*if (baseplate.body.y + baseplate.body.height > scrHeight - scrHeight/4.0 || baseplate.body.y + baseplate.body.height < scrHeight/4.0 ) 
        {
            baseplate.dir.y = -baseplate.dir.y;
        }
        if (baseplate.body.x + baseplate.body.width > scrWidth - scrWidth/4.0 || baseplate.body.x + baseplate.body.width < scrWidth/4.0 ) 
        {
            baseplate.dir.x = -baseplate.dir.x;
        }*/
        baseplate.body.y += baseplate.dir.y;
        baseplate.body.x += baseplate.dir.x;
        

    }
    return 0;
}

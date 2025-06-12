#include <iostream>
#include <raylib.h>
#include <raymath.h>

class Platform
{
    private:
        float coord_array[4];
    
    public:
        Rectangle body; // top left coords, then width & height
        Color color;
        Vector2 dir;
        
        float* body_coords()
        {
            coord_array[0] = body.x;
            coord_array[1] = body.y;
            coord_array[2] = body.width;
            coord_array[3] = body.height;
            return coord_array;
        }        
};

class Player
{
    public:

        Vector2 pos; //x y coords
        Vector2 dir; //x y direction (normalized)
        float speed; //magnitude
        float size;
        Color clr;  

        float x_friction;    
        float x_max_vel, y_max_vel;  

             
        Vector2 updated_pos(float dt)
        {   
            dt *= 60;
            int ux,uy;
            Vector2 upos;
            //dir = Vector2Normalize(dir);  
            if (dir.x > x_max_vel) dir.x = x_max_vel;
            else if (dir.x < -x_max_vel) dir.x = -x_max_vel;

            if (dir.x < 0.1 && dir.x > -0.1) dir.x = 0;
            else dir.x = dir.x / x_friction;
            //dir.y += gravity;
            
            if (dir.y > y_max_vel) dir.y = y_max_vel;
            else if (dir.y < -y_max_vel) dir.y = -y_max_vel;
            else dir.y += 0.1;

            ux = pos.x + (dir.x * speed * dt);
            uy = pos.y + (dir.y * speed * dt);
            upos.x = ux;
            upos.y = uy;
            //std::cout << dt << " \n";

            return upos;
        }

        void border_check(int width, int height)
        {
            //inelastic collision
            //if (pos.x > width || pos.x < 0) dir.x = -dir.x;
            //if (pos.y > height || pos.y < 0) dir.y = -dir.y;

            if (pos.x > width) pos.x = width;
            if (pos.x < 0) pos.x = 0;

            if (pos.y > height) pos.y = height;
            if (pos.y < 0) pos.y = 0;
        }

        void platformCollision(float dt, Vector2* upos,Platform* platforms, int platformsMax )
        {
            for (int i = 0; i < platformsMax; ++i)
            {

            Rectangle checkrect = {upos->x - size,upos->y - size,size*2,size*2};

            if (CheckCollisionRecs(checkrect, platforms[i].body))
            {
                Rectangle overlap = GetCollisionRec(checkrect, platforms[i].body);

                if (overlap.width > overlap.height) //y axis collision
                {
                    if (pos.y < platforms[i].body.y)
                    {
                        pos.x = upos->x;
                        pos.y += platforms[i].dir.y;
                        dir.y = 0;
                        //dir.x = platforms[i]->dir.x;
                    }
                    if (pos.y > platforms[i].body.y + platforms[i].body.height)
                    {
                        pos.x = upos->x;
                        pos.y += platforms[i].dir.y;
                        dir.y = 0;
                    }
                }
                else //x axis
                {
                    if (pos.x < platforms[i].body.x)
                    {
                        pos.y = upos->y;
                        pos.x += platforms[i].dir.x;
                        dir.x = 0;
                    }
                    if (pos.x > platforms[i].body.x + platforms[i].body.width)
                    {
                        pos.y = upos->y;
                        pos.x += platforms[i].dir.x;
                        dir.x = 0;
                    }

                }
                
            }
            else pos = *upos;

            platforms[i].body.y += platforms[i].dir.y;
            platforms[i].body.x += platforms[i].dir.x;
        }
        }

        
};





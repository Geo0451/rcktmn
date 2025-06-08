#include <iostream>
#include <raylib.h>
#include <raymath.h>


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

        
};

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



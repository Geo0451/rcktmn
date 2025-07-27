#include <iostream>
#include <raylib.h>
#include <raymath.h>

class Platform
{ 
    public:
        Rectangle body; // top left coords, then width & height
        Color color;
        Vector2 dir; 
};


class Player
{
    public:
        Vector2 pos; //x y coords
        Vector2 dir; //x y direction (normalized)
        float speed; //magnitude
        float size;
        Color clr;  

        float x_friction;  //technically supposed to be "air resistance"  
        float x_max_vel, y_max_vel;  

        bool canJump;
        
        Vector2 updated_pos(float dt)
        {   //THIS ALSO UPDATES THE DIRECTION VECTOR DIRECTLY
            dt *= 60;
            Vector2 upos;
            //x & y velocity limiter
            dir.x = Clamp(dir.x, -x_max_vel, +x_max_vel);
            dir.y = Clamp(dir.y, -y_max_vel*1.5f, +y_max_vel*2.0f);
            
            if (dir.x < 0.1 && dir.x > -0.1) dir.x = 0;
            else dir.x = dir.x / x_friction;

            return { // upos
                pos.x + (dir.x * speed * dt),
                pos.y + (dir.y * speed * dt) };
        }

        void border_check(int width, int height)
        {
            if (pos.x > width) pos.x = width;
            if (pos.x < 0) pos.x = 0;

            if (pos.y > height) pos.y = height;
            if (pos.y < 0) pos.y = 0;
        }
        
};


class Make
{
    public:
    //bool dragging = false;
    Rectangle selection = {0};

    void select()
    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {   
            selection = {0};
            //dragging = true;
            selection.x = GetMouseX();
            selection.y = GetMouseY();
        };
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            selection.width = GetMouseX() - selection.x;
            selection.height = GetMouseY() - selection.y;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            //dragging = false;
            selection.width = GetMouseX() - selection.x;
            selection.height = GetMouseY() - selection.y;
        }
    }

};


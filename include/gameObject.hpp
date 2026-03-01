#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <math.h>
#include <SDL3/SDL.h>

class GameObject {
public:
    GameObject(const char* tex, float x, float y);
    ~GameObject();

    void update();
    void render();

    float vel = 0.0f;
    float dir = 0.0f;
    float posx, posy;
    double angle = M_PI;
    
private:
    double deg = 0.0;
    SDL_Texture* objTexture = nullptr;
    SDL_FRect srcRect, dstRect;
};

#endif
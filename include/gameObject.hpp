#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <SDL3/SDL.h>
#include <cmath> // Required for M_PI and trigonometry

class GameObject {
public:
    GameObject(const char* tex, float x, float y);
    ~GameObject();

    void update();
    void render();

    float vel = 0.0f;
    float dir = 0.0f;
    float posx, posy;
    double angle = M_PI; // Start facing the correct direction
    
private:
    double deg = 0.0;
    SDL_Texture* objTexture = nullptr;
    SDL_FRect srcRect, dstRect;
};

#endif
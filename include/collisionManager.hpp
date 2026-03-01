#ifndef COLLISIONMANAGER_HPP
#define COLLISIONMANAGER_HPP

#include <SDL3/SDL.h>

class CollisionManager {
public:
    CollisionManager();
    ~CollisionManager();

    // Loads the hitmap image into RAM
    bool loadMask(const char* filePath);
    
    // Frees the memory
    void clean();

    // Collision checks
    bool checkMaskCollision(float carX, float carY, float carWidth, float carHeight) const;
    bool checkAABB(const SDL_FRect& a, const SDL_FRect& b) const;

private:
    SDL_Surface* collisionMask = nullptr;
};

#endif
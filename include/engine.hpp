#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Forward declaration to prevent circular dependency
class GameObject;

class Game {
public:
    Game(const char* title, int width, int height);
    ~Game();
    
    void handleEvents();
    void update();
    void render();
    void clean();
    
    bool running() const { return isRunning; }

    static SDL_FRect camera;
    static SDL_Renderer* renderer;
    
private:
    // Pixel-Perfect Collision components
    SDL_Surface* collisionMask = nullptr;
    bool checkMaskCollision(float carX, float carY, float carWidth, float carHeight);
    
    // Checkpoint collision helper
    bool checkAABB(const SDL_FRect& a, const SDL_FRect& b);

    bool up = false, down = false, braking = false;
    bool isRunning = false;
    
    SDL_Window* window = nullptr;
    GameObject* player = nullptr;
    SDL_Texture* bg = nullptr;
    
    SDL_FRect finishLine;
    SDL_FRect checkpoint;
    bool hitCheckpoint = false;
    Uint64 lapStartTime = 0;
    Uint64 bestLapTime = 0;
};

#endif
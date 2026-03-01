#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "inputManager.hpp"
#include "collisionManager.hpp"

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
    bool isRunning = false;
    bool isDebugMode = false;
    
    SDL_Window* window = nullptr;
    GameObject* player = nullptr;
    SDL_Texture* bg = nullptr;
    
    SDL_FRect finishLine;
    SDL_FRect checkpoint;
    bool hitCheckpoint = false;
    Uint64 lapStartTime = 0;
    Uint64 bestLapTime = 0;

    InputManager input;
    CollisionManager collision;
};

#endif
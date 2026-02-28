#include "engine.hpp"
#include "gameObject.hpp"
#include "textureManager.hpp"
#include "errorLoger.hpp"
#include <iostream>
#include <cmath>

const float MAP_WIDTH = 7680.0f;
const float MAP_HEIGHT = 4320.0f;
const float WINDOW_WIDTH = 1280.0f;
const float WINDOW_HEIGHT = 720.0f;

const float TOP_SPEED = 5.0f;
const float BRAKE_FORCE = 2.0f;
const float ACCELERATION = 0.05f;
const float DEACCELERATION = 0.02f;

SDL_Renderer* Game::renderer = nullptr;
SDL_FRect Game::camera = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};

Game::Game(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        logSDLError(std::cout, "Init");
        isRunning = false;
        return;
    }

    window = SDL_CreateWindow(title, width, height, 0);
    if (window == nullptr) logSDLError(std::cout, "Create window");
    
    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) logSDLError(std::cout, "Create renderer");
    
    // Load visual background
    bg = TextureManager::loadTexture("assets/racetrack.png");
    
    // Load hitmap mask into standard RAM for fast CPU pixel checking
    collisionMask = IMG_Load("assets/mask.png");
    if (collisionMask == nullptr) {
        logSDLError(std::cout, "Load Collision Mask");
    }

    player = new GameObject("assets/redcar.png", 393.0f, 364.0f);
    
    // Define Checkpoints (AABB triggers)
    // Adjust these coordinates to match your specific racetrack
    finishLine = {300.0f, 400.0f, 200.0f, 50.0f}; 
    checkpoint = {4000.0f, 2000.0f, 500.0f, 50.0f}; 

    lapStartTime = SDL_GetTicks();
    isRunning = true;
}

Game::~Game() {}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) isRunning = false;
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_ESCAPE: isRunning = false; break;
                case SDLK_SPACE: braking = true; break;
                case SDLK_A: player->dir = -1.0f; break;
                case SDLK_D: player->dir = 1.0f; break;
                case SDLK_W: up = true; break;
                case SDLK_S: down = true; break;
                default: break;
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP) {
            switch (event.key.key) {
                case SDLK_SPACE: braking = false; break;
                case SDLK_A: 
                case SDLK_D: player->dir = 0.0f; break;
                case SDLK_W: up = false; break;
                case SDLK_S: down = false; break;
                default: break;
            }
        }
    }
}

void Game::update() {
    player->update();
    
    // Center camera on player
    camera.x = player->posx + 32.0f - (WINDOW_WIDTH / 2.0f);
    camera.y = player->posy + 32.0f - (WINDOW_HEIGHT / 2.0f);
    
    // Clamp camera within map bounds
    if (camera.x < 0) camera.x = 0;
    if (camera.x > MAP_WIDTH - camera.w) camera.x = MAP_WIDTH - camera.w;
    if (camera.y < 0) camera.y = 0;
    if (camera.y > MAP_HEIGHT - camera.h) camera.y = MAP_HEIGHT - camera.h;
    
    // --- Mask Collision Check ---
    if (checkMaskCollision(player->posx, player->posy, 64.0f, 64.0f)) {
        // Bounce the car back
        player->vel *= -0.5f; 
        
        // Push the car out of the wall based on its current angle
        player->posx += cos(player->angle) * 5.0f;
        player->posy += sin(player->angle) * 5.0f;
    }

    // --- Lap Timing Logic ---
    SDL_FRect playerBox = {player->posx, player->posy, 64.0f, 64.0f};
    
    if (!hitCheckpoint && checkAABB(playerBox, checkpoint)) {
        hitCheckpoint = true;
        std::cout << "Checkpoint reached!" << std::endl;
    }

    if (hitCheckpoint && checkAABB(playerBox, finishLine)) {
        hitCheckpoint = false;
        Uint64 currentLapTime = SDL_GetTicks() - lapStartTime;
        
        std::cout << "Lap finished! Time: " << currentLapTime / 1000.0f << "s" << std::endl;

        if (bestLapTime == 0 || currentLapTime < bestLapTime) {
            bestLapTime = currentLapTime;
            std::cout << ">>> NEW BEST LAP: " << bestLapTime / 1000.0f << "s <<<" << std::endl;
        }
        
        lapStartTime = SDL_GetTicks();
    }

    // --- Physics ---
    if (up && player->vel < TOP_SPEED) player->vel += ACCELERATION;
    if (down && player->vel > -TOP_SPEED) player->vel -= ACCELERATION;
    
    if (braking && player->vel > 0) player->vel -= BRAKE_FORCE * DEACCELERATION;
    if (braking && player->vel < 0) player->vel += BRAKE_FORCE * DEACCELERATION;
    
    if (!up && player->vel > 0) player->vel -= DEACCELERATION;
    if (!down && player->vel < 0) player->vel += DEACCELERATION;
    
    if (std::abs(player->vel) < 0.02f) player->vel = 0.0f;
}

void Game::render() {
    SDL_RenderClear(renderer);
    
    // Render the background relative to the camera
    SDL_RenderTexture(renderer, bg, &camera, nullptr);
    
    // Render the player
    player->render();
    
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    delete player;
    SDL_DestroyTexture(bg);
    if (collisionMask) SDL_DestroySurface(collisionMask);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Engine cleaned successfully." << std::endl;
}

bool Game::checkMaskCollision(float carX, float carY, float carWidth, float carHeight) {
    if (!collisionMask) return false;

    // Get pixel format details for SDL3
    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(collisionMask->format);
    if (!fmt) return false;
    
    int bpp = fmt->bytes_per_pixel;

    // We check the 4 corners of the car with a small offset (hitbox is slightly smaller than sprite)
    int corners[4][2] = {
        {static_cast<int>(carX + 10), static_cast<int>(carY + 10)},
        {static_cast<int>(carX + carWidth - 10), static_cast<int>(carY + 10)},
        {static_cast<int>(carX + 10), static_cast<int>(carY + carHeight - 10)},
        {static_cast<int>(carX + carWidth - 10), static_cast<int>(carY + carHeight - 10)}
    };

    for (int i = 0; i < 4; i++) {
        int px = corners[i][0];
        int py = corners[i][1];

        // Do not check outside the map boundaries
        if (px < 0 || px >= collisionMask->w || py < 0 || py >= collisionMask->h) {
            continue; 
        }

        // Calculate the exact memory address of the pixel
        Uint8* p = (Uint8*)collisionMask->pixels + py * collisionMask->pitch + px * bpp;
        Uint32 pixelData = 0;

        // Extract the raw pixel data
        switch (bpp) {
            case 1: pixelData = *p; break;
            case 2: pixelData = *(Uint16*)p; break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                    pixelData = p[0] << 16 | p[1] << 8 | p[2];
                else
                    pixelData = p[0] | p[1] << 8 | p[2] << 16;
                break;
            case 4: pixelData = *(Uint32*)p; break;
        }

        // Convert raw data to RGB values using SDL3 function
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixelData, fmt, nullptr, &r, &g, &b, &a);

        // If the pixel is mostly white, it's a collision!
        if (r > 240 && g > 240 && b > 240) {
            return true;
        }
    }
    return false;
}

bool Game::checkAABB(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}
#include "engine.hpp"
#include "gameObject.hpp"
#include "textureManager.hpp"
#include "errorLoger.hpp"
#include <iostream>
#include <cmath>
#include <httplib.h> // Network library
#include <thread>    // Threading for non-blocking requests
#include <string>    // String conversion

constexpr float MAP_WIDTH = 7680.0f;
constexpr float MAP_HEIGHT = 4320.0f;
constexpr float WINDOW_WIDTH = 1280.0f;
constexpr float WINDOW_HEIGHT = 720.0f;

constexpr float TOP_SPEED = 5.0f;
constexpr float BRAKE_FORCE = 2.0f;
constexpr float ACCELERATION = 0.05f;
constexpr float DEACCELERATION = 0.02f;

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

    // Load visual background into VRAM (GPU)
    bg = TextureManager::loadTexture("assets/racetrack.png");

    // --- NEW: Load hitmap mask into standard RAM (CPU) ---
    collision.loadMask("assets/mask.png");
    if (collisionMask == nullptr) {
        logSDLError(std::cout, "Load Collision Mask");
    }

    player = new GameObject("assets/car.png", 393.0f, 364.0f);

    // Checkpoints still use AABB because they are just invisible triggers
    finishLine = {300.0f, 400.0f, 200.0f, 50.0f};
    checkpoint = {2360.0f, 1600.0f, 200.0f, 50.0f};

    lapStartTime = SDL_GetTicks();
    isRunning = true;
}

Game::~Game() = default;

void Game::handleEvents() {
    input.update();

    if (input.isQuitRequested) {
        isRunning = false;
    }
    if (input.isDebugToggled) {
        isDebugMode = !isDebugMode;
        std::cout << "\n[DEVELOPER MODE] " << (isDebugMode ? "ON" : "OFF") << std::endl;
    }
}

void Game::update() {

    // 1. Pass steering directly to the player
    player->dir = input.steeringDir;

    // This guarantees we never get stuck in a wall
    float oldX = player->posx;
    float oldY = player->posy;

    // --- 2. Move Player ---
    player->update();

    // --- 3. Outer Map Boundaries ---
    // Prevent the car from leaving the map dimensions entirely
    bool hitBoundary = false;
    if (player->posx < 0.0f) {
        player->posx = 0.0f;
        hitBoundary = true;
    }
    if (player->posy < 0.0f) {
        player->posy = 0.0f;
        hitBoundary = true;
    }
    if (player->posx + 32.0f > MAP_WIDTH) {
        player->posx = MAP_WIDTH - 32.0f;
        hitBoundary = true;
    }
    if (player->posy + 64.0f > MAP_HEIGHT) {
        player->posy = MAP_HEIGHT - 64.0f;
        hitBoundary = true;
    }

    if (hitBoundary) {
        player->vel *= -0.5f; // Bounce off the map edge
    }

    // --- 4. Mask Collision Check ---
    if (collision.checkMaskCollision(player->posx, player->posy, 32.0f, 64.0f)) {
        // The ultimate fix: instantly revert to the safe position
        player->posx = oldX;
        player->posy = oldY;

        // Bounce the car back
        player->vel *= -0.5f;
    }

    // --- 5. Update Camera ---
    // Center camera perfectly on the 32x64 car
    camera.x = player->posx + 16.0f - (WINDOW_WIDTH / 2.0f);
    camera.y = player->posy + 32.0f - (WINDOW_HEIGHT / 2.0f);

    // Clamp camera within map bounds
    if (camera.x < 0) camera.x = 0;
    if (camera.x > MAP_WIDTH - camera.w) camera.x = MAP_WIDTH - camera.w;
    if (camera.y < 0) camera.y = 0;
    if (camera.y > MAP_HEIGHT - camera.h) camera.y = MAP_HEIGHT - camera.h;

    // --- 6. Lap Timing Logic ---
    SDL_FRect playerBox = {player->posx, player->posy, 32.0f, 64.0f};

    if (!hitCheckpoint && checkAABB(playerBox, checkpoint)) {
        hitCheckpoint = true;
        std::cout << "\n[CHECKPOINT] Reached!" << std::endl;
    }

    if (hitCheckpoint && checkAABB(playerBox, finishLine)) {
        hitCheckpoint = false;
        Uint64 currentLapTime = SDL_GetTicks() - lapStartTime;
        float timeInSeconds = currentLapTime / 1000.0f;

        std::cout << "\n[LAP FINISHED] Time: " << timeInSeconds << "s" << std::endl;

        if (bestLapTime == 0 || currentLapTime < bestLapTime) {
            bestLapTime = currentLapTime;
            std::cout << ">>> NEW BEST LAP: " << timeInSeconds << "s <<<" << std::endl;

            // --- NEW: Send to Kubernetes Backend ---
            // We use a detached thread so the network request doesn't freeze the game
            std::thread([timeInSeconds]() {
                // IMPORTANT: Replace this IP/Port with your actual local Python server or Kubernetes Ingress IP
                httplib::Client cli("http://localhost:4000");

                // Create a simple JSON payload
                std::string payload = "{\"player\": \"Kuba\", \"time\": " + std::to_string(timeInSeconds) + "}";

                // Send the POST request
                if (auto res = cli.Post("/api/laptime", payload, "application/json")) {
                    std::cout << "\n[NETWORK] Successfully sent best lap to server! Status: " << res->status << std::endl;
                } else {
                    std::cout << "\n[NETWORK] Failed to reach the server. Error: " << to_string(res.error()) << std::endl;
                }
            }).detach();
        }

        lapStartTime = SDL_GetTicks();
    }

    // 7. Physics Constraints using InputManager
    if (input.up && player->vel < TOP_SPEED) player->vel += ACCELERATION;
    if (input.down && player->vel > -TOP_SPEED) player->vel -= ACCELERATION;

    if (input.braking && player->vel > 0) player->vel -= BRAKE_FORCE * DEACCELERATION;
    if (input.braking && player->vel < 0) player->vel += BRAKE_FORCE * DEACCELERATION;

    if (!input.up && player->vel > 0) player->vel -= DEACCELERATION;
    if (!input.down && player->vel < 0) player->vel += DEACCELERATION;

    if (std::abs(player->vel) < 0.02f) player->vel = 0.0f;

    // --- 8. DEBUG: Live Telemetry ---
    if (isDebugMode) {
        std::cout << "\r[DEBUG] X: " << static_cast<int>(player->posx)
                  << " | Y: " << static_cast<int>(player->posy)
                  << " | Vel: " << player->vel
                  << " | Deg: " << static_cast<int>(player->angle * (180.0 / M_PI))
                  << "          " << std::flush;
    }
}

void Game::render() {
    SDL_RenderClear(renderer);

    // Render the background relative to the camera
    SDL_RenderTexture(renderer, bg, &camera, nullptr);

    // Render the player texture
    player->render();

    // --- DEVELOPER MODE: Visual Debugging ---
    if (isDebugMode) {
        // Enable alpha blending for semi-transparent boxes
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // 1. Draw Player Bounding Box (Green, Outline)
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_FRect debugBox = { player->posx - camera.x, player->posy - camera.y, 32.0f, 64.0f };
        SDL_RenderRect(renderer, &debugBox);

        // 2. Draw Finish Line (Blue, Semi-transparent fill)
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100);
        SDL_FRect renderFinish = { finishLine.x - camera.x, finishLine.y - camera.y, finishLine.w, finishLine.h };
        SDL_RenderFillRect(renderer, &renderFinish);

        // 3. Draw Checkpoint (Yellow, Semi-transparent fill)
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
        SDL_FRect renderCheckpoint = { checkpoint.x - camera.x, checkpoint.y - camera.y, checkpoint.w, checkpoint.h };
        SDL_RenderFillRect(renderer, &renderCheckpoint);

        // Disable alpha blending to return to normal rendering
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Reset the renderer color back to black for the next frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderPresent(renderer);
}

void Game::clean() {
    delete player;
    SDL_DestroyTexture(bg);

    // --- NEW: Free the mask surface from RAM ---
    if (collisionMask) SDL_DestroySurface(collisionMask);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Engine cleaned successfully." << std::endl;
}

bool Game::checkMaskCollision(float carX, float carY, float carWidth, float carHeight) {
    if (!collisionMask) return false;

    // Get pixel format details (SDL3 specific change)
    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(collisionMask->format);
    if (!fmt) return false;

    int bpp = fmt->bytes_per_pixel;

    // We check the 4 corners of the car
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

        // Convert raw data to RGB values
        Uint8 r, g, b, a;

        // SDL3 requires passing the format details and an optional palette (nullptr for 32-bit PNG)
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
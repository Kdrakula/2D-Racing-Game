#include "engine.hpp"
#include "errorLoger.hpp"
#include "gameConstants.hpp"
#include "gameObject.hpp"
#include "textureManager.hpp"
#include <iostream>
#include <string>

SDL_Renderer *Game::renderer = nullptr;
SDL_FRect Game::camera = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};

Game::Game(const char *title, int width, int height) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    logSDLError(std::cout, "Init");
    isRunning = false;
    return;
  }

  window = SDL_CreateWindow(title, width, height, 0);
  if (window == nullptr)
    logSDLError(std::cout, "Create window");

  renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr)
    logSDLError(std::cout, "Create renderer");

  // Load visual background into VRAM (GPU)
  bg = TextureManager::loadTexture("assets/racetrack.png");

  // --- NEW: Load hitmap mask into standard RAM (CPU) ---
  if (!collision.loadMask("assets/mask.png")) {
    logSDLError(std::cout, "Load Collision Mask");
  }

  player = new GameObject("assets/car.png", 393.0f, 364.0f);

  // Checkpoints and lap timing have been moved to LapTimer

  isRunning = true;
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
    std::cout << "\n[DEVELOPER MODE] " << (isDebugMode ? "ON" : "OFF")
              << std::endl;
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
  if (camera.x < 0)
    camera.x = 0;
  if (camera.x > MAP_WIDTH - camera.w)
    camera.x = MAP_WIDTH - camera.w;
  if (camera.y < 0)
    camera.y = 0;
  if (camera.y > MAP_HEIGHT - camera.h)
    camera.y = MAP_HEIGHT - camera.h;

  // --- 6. Lap Timing Logic ---
  SDL_FRect playerBox = {player->posx, player->posy, 32.0f, 64.0f};
  lapTimer.update(playerBox, WINDOW_WIDTH, WINDOW_HEIGHT, input.playerName);

  // 7. Physics Constraints using InputManager
  if (input.up && player->vel < TOP_SPEED)
    player->vel += ACCELERATION;
  if (input.down && player->vel > -TOP_SPEED)
    player->vel -= ACCELERATION;

  if (input.braking && player->vel > 0)
    player->vel -= BRAKE_FORCE * DEACCELERATION;
  if (input.braking && player->vel < 0)
    player->vel += BRAKE_FORCE * DEACCELERATION;

  if (!input.up && player->vel > 0)
    player->vel -= DEACCELERATION;
  if (!input.down && player->vel < 0)
    player->vel += DEACCELERATION;

  if (std::abs(player->vel) < 0.02f)
    player->vel = 0.0f;

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

  lapTimer.renderTime(renderer, WINDOW_WIDTH);

  if (input.isTypingName) {
    std::string prompt = "Type Nickname: " + input.playerName + "_";
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_FRect bgRect = {(WINDOW_WIDTH - 300.0f) / 2.0f, 60.0f, 300.0f, 40.0f};
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_RenderDebugText(renderer, bgRect.x + 20, bgRect.y + 10, prompt.c_str());
  }

  // --- DEVELOPER MODE: Visual Debugging ---
  if (isDebugMode) {
    // Enable alpha blending for semi-transparent boxes
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // 1. Draw Player Bounding Box (Green, Outline)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect debugBox = {player->posx - camera.x, player->posy - camera.y,
                          32.0f, 64.0f};
    SDL_RenderRect(renderer, &debugBox);

    lapTimer.renderDebug(renderer, camera);

    // Disable alpha blending to return to normal rendering
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  // --- LEADERBOARD OVERLAY ---
  if (input.showResults) {
    lapTimer.renderResults(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
  }

  // Reset the renderer color back to black for the next frame
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_RenderPresent(renderer);
}

void Game::clean() {
  delete player;
  SDL_DestroyTexture(bg);

  // Mask is freed automatically in CollisionManager's destructor

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::cout << "Engine cleaned successfully." << std::endl;
}

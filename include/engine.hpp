#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "collisionManager.hpp"
#include "debugOverlay.hpp"
#include "gameConstants.hpp"
#include "inputManager.hpp"
#include "lapTimer.hpp"
#include "overlay.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <vector>

class GameObject;

class Game {
public:
  Game(const char *title, int width, int height, const TrackInfo &track);
  ~Game();

  void handleEvents();
  void update();
  void render();
  void clean();

  bool running() const { return isRunning; }

  static SDL_FRect camera;
  static SDL_Renderer *renderer;
  static TTF_Font *font;

private:
  bool isRunning = false;
  bool isDebugMode = false;

  SDL_Window *window = nullptr;
  GameObject *player = nullptr;
  SDL_Texture *bg = nullptr;
  SDL_Texture *maskTexture = nullptr;

  SDL_FRect playerBox_ = {0, 0, 32.0f, 64.0f};

  InputManager input;
  CollisionManager collision;
  LapTimer lapTimer;

  std::vector<std::unique_ptr<Overlay>> overlays;
  DebugOverlay *debugOverlay_ = nullptr; // non-owning, for telemetry

  TrackInfo track_;

  // --- Auto-Updater ---
  bool updateAvailable = false;
  std::string updateDownloadUrl;
  void checkForUpdates();
};

#endif
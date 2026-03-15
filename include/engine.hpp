#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "collisionManager.hpp"
#include "inputManager.hpp"
#include "lapTimer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

class GameObject;

class Game {
public:
  Game(const char *title, int width, int height);
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

  InputManager input;
  CollisionManager collision;
  LapTimer lapTimer;

  // --- Auto-Updater ---
  bool updateAvailable = false;
  std::string updateDownloadUrl;
  void checkForUpdates();
};

#endif
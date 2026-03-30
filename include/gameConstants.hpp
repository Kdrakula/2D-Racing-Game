#ifndef GAMECONSTANTS_HPP
#define GAMECONSTANTS_HPP

#include <string>
#include <vector>
#include <SDL3/SDL.h>

constexpr float MAP_WIDTH = 7680.0f;
constexpr float MAP_HEIGHT = 4320.0f;
constexpr float WINDOW_WIDTH = 1280.0f;
constexpr float WINDOW_HEIGHT = 720.0f;

// Physics constants
constexpr float TOP_SPEED = 5.0f;
constexpr float BRAKE_FORCE = 2.0f;
constexpr float ACCELERATION = 0.05f;
constexpr float DEACCELERATION = 0.02f;

struct TrackInfo {
  std::string name;
  std::string bgAsset;   // e.g. "assets/track-1.png"
  std::string maskAsset; // e.g. "assets/maks-1.png"
  float startX;
  float startY;
  SDL_FRect finishLine;
  std::vector<SDL_FRect> checkpoints;
};

#endif

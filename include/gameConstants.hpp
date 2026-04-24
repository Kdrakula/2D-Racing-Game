#ifndef GAMECONSTANTS_HPP
#define GAMECONSTANTS_HPP

#include <SDL3/SDL.h>
#include <string>
#include <vector>

constexpr float MAP_WIDTH = 7680.0f;
constexpr float MAP_HEIGHT = 4320.0f;
constexpr float WINDOW_WIDTH = 1280.0f;
constexpr float WINDOW_HEIGHT = 720.0f;

// Network constants
const std::vector<std::string> API_URLS = {
    "http://100.71.178.10:4000",   // Tailscale
    "http://beaglebone.local:4000", // Local BeagleBone
    "http://127.0.0.1:4000"         // Local Orbstack
};

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
  double startAngle;
  SDL_FRect finishLine;
  std::vector<SDL_FRect> checkpoints;
};

#endif

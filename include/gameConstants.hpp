#ifndef GAMECONSTANTS_HPP
#define GAMECONSTANTS_HPP

#include <string>

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
  std::string bgAsset;   // e.g. "assets/racetrack.png"
  std::string maskAsset; // e.g. "assets/mask.png"
  float startX;
  float startY;
};

#endif

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <SDL3/SDL.h>
#include <string>

class InputManager {
public:
  InputManager();
  ~InputManager();

  // Call this once per frame
  void update();

  // Public state variables
  bool isQuitRequested = false;
  bool isDebugToggled = false;
  bool showMaskToggled = false;
  bool showResults = false;

  bool showMask = false;

  bool isTypingName = false;
  std::string playerName = "Player";

  bool up = false;
  bool down = false;
  bool braking = false;
  bool updateKey = false;
  float steeringDir = 0.0f;
};

#endif
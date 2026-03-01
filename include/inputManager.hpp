#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <SDL3/SDL.h>

class InputManager {
public:
    InputManager();
    ~InputManager();

    // Call this once per frame
    void update();

    // Public state variables
    bool isQuitRequested = false;
    bool isDebugToggled = false;
    
    bool up = false;
    bool down = false;
    bool braking = false;
    float steeringDir = 0.0f;
};

#endif
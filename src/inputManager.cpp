#include "inputManager.hpp"

InputManager::InputManager() {}

InputManager::~InputManager() {}

void InputManager::update() {
    // Reset one-frame flags
    isDebugToggled = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isQuitRequested = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_ESCAPE: isQuitRequested = true; break;
                case SDLK_TAB: isDebugToggled = true; break;
                case SDLK_SPACE: braking = true; break;
                case SDLK_A: steeringDir = -1.0f; break;
                case SDLK_D: steeringDir = 1.0f; break;
                case SDLK_W: up = true; break;
                case SDLK_S: down = true; break;
                default: break;
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP) {
            switch (event.key.key) {
                case SDLK_SPACE: braking = false; break;
                case SDLK_A: 
                case SDLK_D: steeringDir = 0.0f; break;
                case SDLK_W: up = false; break;
                case SDLK_S: down = false; break;
                default: break;
            }
        }
    }
}
#include "inputManager.hpp"
#include <fstream>
#include <SDL3/SDL.h>
#include <random>
#include <sstream>
#include <iostream>

static std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

InputManager::InputManager() {
    std::string path = std::string(SDL_GetBasePath()) + "assets/player_name.txt";
    std::ifstream in(path);
    if (in.is_open()) {
        std::getline(in, playerName);
        in.close();
    }

    std::string uuidPath = std::string(SDL_GetBasePath()) + "assets/client_id.txt";
    std::ifstream uuidIn(uuidPath);
    if (uuidIn.is_open()) {
        std::getline(uuidIn, clientId);
        uuidIn.close();
    } else {
        clientId = generateUUID();
        std::ofstream uuidOut(uuidPath);
        if (uuidOut.is_open()) {
            uuidOut << clientId;
            uuidOut.close();
        }
    }
}

InputManager::~InputManager() {}

void InputManager::update() {
  // Reset one-frame flags
  isDebugToggled = false;
  showMaskToggled = false;
  refreshLeaderboard = false;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      isQuitRequested = true;
      isHardQuitRequested = true;
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
      if (isTypingName) {
        switch (event.key.key) {
        case SDLK_ESCAPE:
          isQuitRequested = true;
          break;
        case SDLK_BACKSPACE:
          if (!playerName.empty()) {
            playerName.pop_back();
          }
          break;
        case SDLK_RETURN:
          isTypingName = false;
          SDL_StopTextInput(SDL_GetKeyboardFocus());
          {
              std::string path = std::string(SDL_GetBasePath()) + "assets/player_name.txt";
              std::ofstream out(path);
              if (out.is_open()) {
                  out << playerName;
                  out.close();
              }
          }
          break;
        default:
          break;
        }
      } else {
        switch (event.key.key) {
        case SDLK_ESCAPE:
          isQuitRequested = true;
          break;
        case SDLK_V:
          showNames = !showNames;
          break;
        case SDLK_F:
          showFps = !showFps;
          break;
        case SDLK_L:
          isDebugToggled = true;
          break;
        case SDLK_M:
          showMaskToggled = true;
          break;
        case SDLK_TAB:
          if (!showResults) refreshLeaderboard = true;
          showResults = true;
          break;
        case SDLK_N:
          isTypingName = true;
          SDL_StartTextInput(SDL_GetKeyboardFocus());
          break;
        case SDLK_SPACE:
          braking = true;
          break;
        case SDLK_U:
          updateKey = true;
          break;
        case SDLK_A:
          steeringDir = -1.0f;
          break;
        case SDLK_D:
          steeringDir = 1.0f;
          break;
        case SDLK_W:
          up = true;
          break;
        case SDLK_S:
          down = true;
          break;
        default:
          break;
        }
      }
    } else if (event.type == SDL_EVENT_KEY_UP) {
      switch (event.key.key) {
      case SDLK_TAB:
        showResults = false;
        break;
      case SDLK_SPACE:
        braking = false;
        break;
      case SDLK_U:
        updateKey = false;
        break;
      case SDLK_A:
      case SDLK_D:
        steeringDir = 0.0f;
        break;
      case SDLK_W:
        up = false;
        break;
      case SDLK_S:
        down = false;
        break;
      default:
        break;
      }
    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
      if (isTypingName && playerName.length() < 15) {
        playerName += event.text.text;
      }
    }
  }
}
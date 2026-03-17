#include "nicknameOverlay.hpp"
#include "gameConstants.hpp"
#include <SDL3/SDL.h>

NicknameOverlay::NicknameOverlay(const bool &isTyping,
                                 const std::string &playerName)
    : isTyping_(isTyping), playerName_(playerName) {}

bool NicknameOverlay::isActive() const { return isTyping_; }

void NicknameOverlay::render(SDL_Renderer *renderer, TTF_Font * /*font*/) {
  std::string prompt = "Type Nickname: " + playerName_ + "_";

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
  SDL_FRect bgRect = {(WINDOW_WIDTH - 300.0f) / 2.0f, 60.0f, 300.0f, 40.0f};
  SDL_RenderFillRect(renderer, &bgRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  SDL_RenderDebugText(renderer, bgRect.x + 20, bgRect.y + 10, prompt.c_str());
}

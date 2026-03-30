#include "debugOverlay.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DebugOverlay::DebugOverlay(const bool &active, const SDL_FRect &playerBox,
                           const SDL_FRect &camera, LapTimer &lapTimer,
                           const float &vel, const float &angle,
                           const std::string &version)
    : active_(active), playerBox_(playerBox), camera_(camera),
      lapTimer_(lapTimer), vel_(vel), angle_(angle), version_(version) {}

bool DebugOverlay::isActive() const { return active_; }

void DebugOverlay::renderTelemetry() const {
  std::cout << "\r[DEBUG] X: " << static_cast<int>(playerBox_.x)
            << " | Y: " << static_cast<int>(playerBox_.y) << " | Vel: " << vel_
            << " | Deg: " << static_cast<int>(angle_ * (180.0 / M_PI))
            << "          " << std::flush;
}

void DebugOverlay::render(SDL_Renderer *renderer, TTF_Font *font) {
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  // Green player bounding box
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_FRect debugBox = {playerBox_.x - camera_.x, playerBox_.y - camera_.y,
                        playerBox_.w, playerBox_.h};
  SDL_RenderRect(renderer, &debugBox);


  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // Yellow version text (top-left)
  if (font) {
    SDL_Color versionColor = {255, 255, 0, 255};
    std::string verText = "Developer Mode - Version: " + version_;

    SDL_Surface *surf =
        TTF_RenderText_Blended(font, verText.c_str(), 0, versionColor);
    if (surf) {
      SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
      SDL_DestroySurface(surf);
      if (tex) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tex, &tw, &th);
        SDL_FRect dst = {10.0f, 10.0f, tw, th};
        SDL_RenderTexture(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
      } else {
        std::cout << "[DEBUG] Failed to create texture: " << SDL_GetError()
                  << std::endl;
      }
    } else {
      std::cout << "[DEBUG] Failed to create surface: " << SDL_GetError()
                << std::endl;
    }
  }
}

#include "debugOverlay.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DebugOverlay::DebugOverlay(const bool &active, const SDL_FRect &playerBox,
                           const SDL_FRect &camera, LapTimer &lapTimer,
                           const float &vel, const double &angle,
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

  // Green player bounding box (Oriented)
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  
  float cx = playerBox_.x + playerBox_.w / 2.0f - camera_.x;
  float cy = playerBox_.y + playerBox_.h / 2.0f - camera_.y;

  double theta_rad = angle_ - M_PI / 2.0;
  float cos_theta = std::cos(theta_rad);
  float sin_theta = std::sin(theta_rad);

  float hw = playerBox_.w / 2.0f;
  float hh = playerBox_.h / 2.0f;

  float offsets[4][2] = {
      {-hw + 4.0f, -hh + 8.0f}, // 0: Top-Left
      { hw - 4.0f, -hh + 8.0f}, // 1: Top-Right
      { hw - 4.0f,  hh - 8.0f}, // 2: Bottom-Right
      {-hw + 4.0f,  hh - 8.0f}  // 3: Bottom-Left
  };

  SDL_FPoint points[5];
  for (int i = 0; i < 4; i++) {
      float u = offsets[i][0];
      float v = offsets[i][1];

      float rot_u = u * cos_theta - v * sin_theta;
      float rot_v = u * sin_theta + v * cos_theta;

      points[i].x = cx + rot_u;
      points[i].y = cy + rot_v;
  }
  points[4] = points[0]; // Close the loop

  SDL_RenderLines(renderer, points, 5);


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

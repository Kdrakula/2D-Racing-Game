#ifndef DEBUGOVERLAY_HPP
#define DEBUGOVERLAY_HPP

#include "lapTimer.hpp"
#include "overlay.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class DebugOverlay : public Overlay {
public:
  DebugOverlay(const bool &active, const SDL_FRect &playerBox,
               const SDL_FRect &camera, LapTimer &lapTimer, const float &vel,
               const float &angle, const std::string &version);

  void render(SDL_Renderer *renderer, TTF_Font *font) override;
  bool isActive() const override;

  // Separate stdout telemetry (called from update(), not render())
  void renderTelemetry() const;

private:
  const bool &active_;
  const SDL_FRect &playerBox_;
  const SDL_FRect &camera_;
  LapTimer &lapTimer_;
  const float &vel_;
  const float &angle_;
  std::string version_;
};

#endif

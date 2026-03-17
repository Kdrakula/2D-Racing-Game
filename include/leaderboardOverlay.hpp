#ifndef LEADERBOARDOVERLAY_HPP
#define LEADERBOARDOVERLAY_HPP

#include "lapTimer.hpp"
#include "overlay.hpp"

class LeaderboardOverlay : public Overlay {
public:
  LeaderboardOverlay(const bool &showResults, LapTimer &lapTimer);

  void render(SDL_Renderer *renderer, TTF_Font *font) override;
  bool isActive() const override;

private:
  const bool &showResults_;
  LapTimer &lapTimer_;
};

#endif

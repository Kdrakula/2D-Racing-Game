#ifndef UPDATEROVERLAY_HPP
#define UPDATEROVERLAY_HPP

#include "overlay.hpp"

class UpdaterOverlay : public Overlay {
public:
  explicit UpdaterOverlay(const bool &updateAvailable);

  void render(SDL_Renderer *renderer, TTF_Font *font) override;
  bool isActive() const override;

private:
  const bool &updateAvailable_;
};

#endif

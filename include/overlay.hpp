#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class Overlay {
public:
  virtual ~Overlay() = default;
  virtual void render(SDL_Renderer *renderer, TTF_Font *font) = 0;
  virtual bool isActive() const = 0;
};

#endif

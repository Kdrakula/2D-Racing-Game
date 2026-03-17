#include "updaterOverlay.hpp"
#include "gameConstants.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

UpdaterOverlay::UpdaterOverlay(const bool &updateAvailable)
    : updateAvailable_(updateAvailable) {}

bool UpdaterOverlay::isActive() const { return updateAvailable_; }

void UpdaterOverlay::render(SDL_Renderer *renderer, TTF_Font *font) {
  if (!font) return;

  SDL_Color textColor = {255, 50, 50, 255};
  SDL_Surface *surf = TTF_RenderText_Blended(
      font, "New Update Available! Press U to Restart and Download.", 0,
      textColor);
  if (surf) {
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
      SDL_FRect dst = {(WINDOW_WIDTH - surf->w) / 2.0f, 60.0f,
                       static_cast<float>(surf->w),
                       static_cast<float>(surf->h)};
      SDL_RenderTexture(renderer, tex, nullptr, &dst);
      SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(surf);
  }
}

#ifndef MENU_HPP
#define MENU_HPP

#include "gameConstants.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>

class Menu {
public:
  Menu();
  ~Menu();

  // Run the full menu loop (init → event loop → cleanup).
  // Returns true if the user confirmed a selection, false if they quit.
  bool run();

  TrackInfo selectedTrack() const;

private:
  void init();
  void handleEvents();
  void render();
  void clean();

  std::string resolveAsset(const std::string &relative) const;

  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  TTF_Font *titleFont_ = nullptr;
  TTF_Font *bodyFont_ = nullptr;

  std::vector<TrackInfo> tracks_;
  std::vector<SDL_Texture *> thumbnails_;

  int selectedIndex_ = 0;
  float continuousIndex_ = 0.0f;
  bool confirmed_ = false;
  bool running_ = false;
};

#endif
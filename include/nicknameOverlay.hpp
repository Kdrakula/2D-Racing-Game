#ifndef NICKNAMEOVERLAY_HPP
#define NICKNAMEOVERLAY_HPP

#include "overlay.hpp"
#include <string>

class NicknameOverlay : public Overlay {
public:
  NicknameOverlay(const bool &isTyping, const std::string &playerName);

  void render(SDL_Renderer *renderer, TTF_Font *font) override;
  bool isActive() const override;

private:
  const bool &isTyping_;
  const std::string &playerName_;
};

#endif

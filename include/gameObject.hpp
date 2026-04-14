#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <SDL3/SDL.h>

class GameObject {
public:
  GameObject(const char *tex, float x, float y, double startAngle);
  ~GameObject();

  void update();
  void render();

  SDL_Texture* getTexture() const { return objTexture; }

  float vel = 0.0f;
  float dir = 0.0f;
  float posx, posy;
  double angle;
  float width = 64.0f;
  float height = 64.0f;

private:
  double deg = 0.0;
  SDL_Texture *objTexture = nullptr;
  SDL_FRect srcRect, dstRect;
};

#endif
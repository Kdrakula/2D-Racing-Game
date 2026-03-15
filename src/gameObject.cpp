#include "gameObject.hpp"
#include "engine.hpp"
#include "textureManager.hpp"
#include <cmath>

const float SPEED = 2.0f;
const float STEERING_SPEED = 0.05f;
const float SCALE = 1.0f;

GameObject::GameObject(const char *tex, float x, float y) {
  objTexture = TextureManager::loadTexture(tex);
  posx = x;
  posy = y;
  angle = M_PI;
  srcRect = {0.0f, 0.0f, 64.0f, 64.0f};
  dstRect = {0.0f, 0.0f, 64.0f * SCALE, 64.0f * SCALE};
}

GameObject::~GameObject() {
  if (objTexture != nullptr) {
    SDL_DestroyTexture(objTexture);
  }
}

void GameObject::update() {
  // Steer only if moving
  if (vel != 0.0f) {
    angle = fmod(angle + dir * STEERING_SPEED, 2.0 * M_PI);
  }

  deg = angle * (180.0 / M_PI);

  // THE FIX: Restored original subtraction so the car moves forward correctly
  posx -= SPEED * (cos(angle) * vel);
  posy -= SPEED * (sin(angle) * vel);

  // Render relative to camera position
  dstRect.x = posx - Game::camera.x;
  dstRect.y = posy - Game::camera.y;
}

void GameObject::render() {
  SDL_RenderTextureRotated(Game::renderer, objTexture, &srcRect, &dstRect,
                           (deg - 90), nullptr, SDL_FLIP_NONE);
}
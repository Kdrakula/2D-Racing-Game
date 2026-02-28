#include "textureManager.hpp"
#include "engine.hpp"
#include "errorLoger.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>

SDL_Texture* TextureManager::loadTexture(const char* fileName) {
    SDL_Texture* texture = IMG_LoadTexture(Game::renderer, fileName);
    if (texture == nullptr) {
        logSDLError(std::cout, "Load Texture");
    }
    return texture;
}
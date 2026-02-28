#ifndef TEXTUREMANAGER_HPP
#define TEXTUREMANAGER_HPP

#include <SDL3/SDL.h>

class TextureManager {
public:
    static SDL_Texture* loadTexture(const char* fileName);
};

#endif
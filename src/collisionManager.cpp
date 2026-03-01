#include "collisionManager.hpp"
#include "errorLoger.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>

CollisionManager::CollisionManager() {}

CollisionManager::~CollisionManager() {
    clean();
}

bool CollisionManager::loadMask(const char* filePath) {
    collisionMask = IMG_Load(filePath);
    if (collisionMask == nullptr) {
        logSDLError(std::cout, "Load Collision Mask");
        return false;
    }
    return true;
}

void CollisionManager::clean() {
    if (collisionMask) {
        SDL_DestroySurface(collisionMask);
        collisionMask = nullptr;
    }
}

bool CollisionManager::checkMaskCollision(float carX, float carY, float carWidth, float carHeight) const {
    if (!collisionMask) return false;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(collisionMask->format);
    if (!fmt) return false;
    
    int bpp = fmt->bytes_per_pixel;

    // Check the 4 corners of the 32x64 car (with a 4px/8px offset)
    int corners[4][2] = {
        {static_cast<int>(carX + 4), static_cast<int>(carY + 8)},
        {static_cast<int>(carX + carWidth - 4), static_cast<int>(carY + 8)},
        {static_cast<int>(carX + 4), static_cast<int>(carY + carHeight - 8)},
        {static_cast<int>(carX + carWidth - 4), static_cast<int>(carY + carHeight - 8)}
    };

    for (int i = 0; i < 4; i++) {
        int px = corners[i][0];
        int py = corners[i][1];

        // Bounds check
        if (px < 0 || px >= collisionMask->w || py < 0 || py >= collisionMask->h) {
            continue; 
        }

        // Memory address calculation
        Uint8* p = (Uint8*)collisionMask->pixels + py * collisionMask->pitch + px * bpp;
        Uint32 pixelData = 0;

        switch (bpp) {
            case 1: pixelData = *p; break;
            case 2: pixelData = *(Uint16*)p; break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                    pixelData = p[0] << 16 | p[1] << 8 | p[2];
                else
                    pixelData = p[0] | p[1] << 8 | p[2] << 16;
                break;
            case 4: pixelData = *(Uint32*)p; break;
        }

        Uint8 r, g, b, a;
        SDL_GetRGBA(pixelData, fmt, nullptr, &r, &g, &b, &a);

        // White pixel means collision
        if (r > 240 && g > 240 && b > 240) {
            return true;
        }
    }
    return false;
}

bool CollisionManager::checkAABB(const SDL_FRect& a, const SDL_FRect& b) const {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}
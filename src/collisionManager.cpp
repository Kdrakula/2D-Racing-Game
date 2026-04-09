#include "collisionManager.hpp"
#include "errorLoger.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <cmath>

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

bool CollisionManager::checkMaskCollision(float carX, float carY, float carWidth, float carHeight, double angle) const {
    if (!collisionMask) return false;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(collisionMask->format);
    if (!fmt) return false;
    
    int bpp = fmt->bytes_per_pixel;

    float cx = carX + carWidth / 2.0f;
    float cy = carY + carHeight / 2.0f;

    // The rendering angle is angle - M_PI/2 radians, meaning the graphic is rotated
    // clockwise by this amount. We rotate the offset points by the same amount.
    double theta_rad = angle - M_PI / 2.0;

    float cos_theta = std::cos(theta_rad);
    float sin_theta = std::sin(theta_rad);

    float hw = carWidth / 2.0f;
    float hh = carHeight / 2.0f;

    // Offsets from the center of the car (with 4px horizontal, 8px vertical padding).
    float offsets[4][2] = {
        {-hw + 4.0f, -hh + 8.0f},
        { hw - 4.0f, -hh + 8.0f},
        {-hw + 4.0f,  hh - 8.0f},
        { hw - 4.0f,  hh - 8.0f}
    };

    for (int i = 0; i < 4; i++) {
        float u = offsets[i][0];
        float v = offsets[i][1];

        float rot_u = u * cos_theta - v * sin_theta;
        float rot_v = u * sin_theta + v * cos_theta;

        int px = static_cast<int>(cx + rot_u);
        int py = static_cast<int>(cy + rot_v);

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
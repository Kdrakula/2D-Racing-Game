#include "ghostManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <SDL3/SDL.h>

GhostManager::GhostManager() {
    playbackIndex_ = 0;
}

GhostManager::~GhostManager() {
}

void GhostManager::startRecording() {
    currentLapFrames_.clear();
    currentLapFrames_.reserve(3600); // Reserve memory for ~1 minute lap at 60fps
}

void GhostManager::recordFrame(Uint32 lapTimeMs, float x, float y, double angle) {
    GhostFrame frame = {lapTimeMs, x, y, angle};
    currentLapFrames_.push_back(frame);
}

void GhostManager::saveBestLap(const std::string& trackName) {
    if (currentLapFrames_.empty()) return;

    bestLapFrames_ = currentLapFrames_;
    playbackIndex_ = 0;

    std::cout << "[GHOST] New best lap saved locally (" << bestLapFrames_.size() << " frames).\n";

    // Save to file
    std::string dir = "assets/ghosts";
    std::filesystem::create_directories(dir);

    // Sanitize trackName for filename
    std::string safeName = trackName;
    for (char& c : safeName) {
        if (c == ' ' || c == '/' || c == '\\') c = '_';
    }

    std::string path = dir + "/" + safeName + ".ghost";
    std::ofstream out(path, std::ios::binary);
    if (out.is_open()) {
        size_t count = bestLapFrames_.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(bestLapFrames_.data()), count * sizeof(GhostFrame));
        out.close();
    } else {
        std::cerr << "[GHOST] Failed to save ghost data to " << path << std::endl;
    }
}

void GhostManager::loadGhost(const std::string& trackName) {
    bestLapFrames_.clear();
    playbackIndex_ = 0;

    std::string safeName = trackName;
    for (char& c : safeName) {
        if (c == ' ' || c == '/' || c == '\\') c = '_';
    }
    std::string path = "assets/ghosts/" + safeName + ".ghost";

    std::ifstream in(path, std::ios::binary);
    if (in.is_open()) {
        size_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (count > 0 && count < 1000000) { // arbitrary limit, prevents bad reads
            bestLapFrames_.resize(count);
            in.read(reinterpret_cast<char*>(bestLapFrames_.data()), count * sizeof(GhostFrame));
            std::cout << "[GHOST] Loaded ghost from " << path << " (" << count << " frames).\n";
        }
        in.close();
    }
}

void GhostManager::resetRecording() {
    currentLapFrames_.clear();
}

bool GhostManager::getGhostState(Uint32 currentLapTimeMs, float& outX, float& outY, double& outAngle) {
    if (bestLapFrames_.empty()) return false;

    // The recorded lap times are strictly increasing.
    // We linearly interpolate between two frames.

    // If lap is shorter than target, stay at finish.
    if (currentLapTimeMs >= bestLapFrames_.back().lapTimeMs) {
        outX = bestLapFrames_.back().x;
        outY = bestLapFrames_.back().y;
        outAngle = bestLapFrames_.back().angle;
        return true;
    }

    // Optimization: find index continuously
    while (playbackIndex_ < bestLapFrames_.size() - 1 && 
           bestLapFrames_[playbackIndex_ + 1].lapTimeMs <= currentLapTimeMs) {
        playbackIndex_++;
    }

    // if lap time reset (new lap starts)
    if (playbackIndex_ > 0 && currentLapTimeMs < bestLapFrames_[playbackIndex_].lapTimeMs) {
        playbackIndex_ = 0;
        while (playbackIndex_ < bestLapFrames_.size() - 1 && 
               bestLapFrames_[playbackIndex_ + 1].lapTimeMs <= currentLapTimeMs) {
            playbackIndex_++;
        }
    }

    const GhostFrame& f1 = bestLapFrames_[playbackIndex_];
    
    if (playbackIndex_ + 1 < bestLapFrames_.size()) {
        const GhostFrame& f2 = bestLapFrames_[playbackIndex_ + 1];
        float t = 0.0f;
        Uint32 dur = f2.lapTimeMs - f1.lapTimeMs;
        if (dur > 0) {
            t = static_cast<float>(currentLapTimeMs - f1.lapTimeMs) / static_cast<float>(dur);
        }
        outX = f1.x + (f2.x - f1.x) * t;
        outY = f1.y + (f2.y - f1.y) * t;
        
        // Simple angle interpolation (might spin if crossed Pi boundaries, but standard track movements are okay unless >180 degree snaps)
        // A proper way is lerping the shortest path.
        double a1 = f1.angle;
        double a2 = f2.angle;
        double diff = a2 - a1;
        while (diff < -M_PI) diff += 2.0 * M_PI;
        while (diff > M_PI) diff -= 2.0 * M_PI;
        
        outAngle = a1 + diff * t;
    } else {
        outX = f1.x;
        outY = f1.y;
        outAngle = f1.angle;
    }

    return true;
}

void GhostManager::render(SDL_Renderer* renderer, SDL_Texture* carTexture, const SDL_FRect& camera, Uint32 currentLapTimeMs) {
    if (!renderer || !carTexture || bestLapFrames_.empty()) return;

    float gx, gy;
    double gAngle;
    if (getGhostState(currentLapTimeMs, gx, gy, gAngle)) {
        // Source and destination rects matched to GameObject size
        SDL_FRect srcRect = {0.0f, 0.0f, 32.0f, 64.0f};
        SDL_FRect dstRect = {gx - camera.x, gy - camera.y, 32.0f, 64.0f};
        
        // Save existing mod state
        SDL_BlendMode oldBlendMode;
        Uint8 oldR, oldG, oldB, oldA;
        SDL_GetTextureBlendMode(carTexture, &oldBlendMode);
        SDL_GetTextureColorMod(carTexture, &oldR, &oldG, &oldB);
        SDL_GetTextureAlphaMod(carTexture, &oldA);

        // Render slightly blue and transparent
        SDL_SetTextureColorMod(carTexture, 100, 150, 255);
        SDL_SetTextureBlendMode(carTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(carTexture, 120);

        double deg = gAngle * (180.0 / M_PI);
        SDL_RenderTextureRotated(renderer, carTexture, &srcRect, &dstRect, (deg - 90), nullptr, SDL_FLIP_NONE);

        // Restoring exactly what the texture had before
        SDL_SetTextureColorMod(carTexture, oldR, oldG, oldB);
        SDL_SetTextureAlphaMod(carTexture, oldA);
        SDL_SetTextureBlendMode(carTexture, oldBlendMode);
    }
}

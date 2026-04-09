#ifndef GHOSTMANAGER_HPP
#define GHOSTMANAGER_HPP

#include <SDL3/SDL.h>
#include <vector>
#include <string>

struct GhostFrame {
    Uint32 lapTimeMs;
    float x;
    float y;
    double angle;
};

class GhostManager {
public:
    GhostManager();
    ~GhostManager();

    // Start recording a new lap
    void startRecording();

    // Record the player's position at a specific time in the current lap
    void recordFrame(Uint32 lapTimeMs, float x, float y, double angle);

    // Save the current lap as the best lap, and optimally save to local disk
    void saveBestLap(const std::string& trackName);

    // Load a ghost from the local disk for a specific track
    void loadGhost(const std::string& trackName);

    // Reset recording if lap is canceled
    void resetRecording();

    // Update and return the interpolated ghost state based on current lap time
    bool getGhostState(Uint32 currentLapTimeMs, float& outX, float& outY, double& outAngle);

    // Render the ghost car
    void render(SDL_Renderer* renderer, SDL_Texture* carTexture, const SDL_FRect& camera, Uint32 currentLapTimeMs);

private:
    std::vector<GhostFrame> currentLapFrames_;
    std::vector<GhostFrame> bestLapFrames_;

    // To optimize lookup, track the last accessed index in the playback
    size_t playbackIndex_ = 0;
};

#endif

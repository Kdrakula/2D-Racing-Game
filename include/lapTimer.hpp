#ifndef LAPTIMER_HPP
#define LAPTIMER_HPP

#include "gameConstants.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <mutex>

class GhostManager;

class LapTimer {
public:
  LapTimer(GhostManager* gm = nullptr);

  // Pass the player's bounding box and track info
  // update the lap state based on player position.
  // Returns: 0 = running, 1 = started/restarted lap, 2 = finished lap, 3 = finished (new best)
  int update(const SDL_FRect &playerBox, const TrackInfo &track, const std::string &playerName);

  bool isStarted() const { return isLapStarted; }
  Uint32 getCurrentLapTimeMs() const;
  // Draw the finish line and checkpoints when debug is true
  void render(SDL_Renderer *renderer, const SDL_FRect &camera, bool debug,
              const TrackInfo &track);

  // Render on-screen timer
  void renderTime(SDL_Renderer *renderer, float windowWidth);

  // Draw leaderboard overlay when user holds TAB
  void renderResults(SDL_Renderer *renderer, float windowWidth,
                     float windowHeight);

  // Sync world record ghost if available
  void fetchLeaderboard(const std::string &trackName);

private:
  SDL_FRect finishLine;
  std::vector<SDL_FRect> checkpoints;
  std::vector<bool> hitCheckpoints;
  std::string currentTrackName;

  Uint64 startTime;
  float lastLapTime;
  float bestLapTime;
  bool isLapStarted;
  bool bestLapSet;

  // Leaderboard data
  struct LapRecord {
    std::string player;
    float time;
    std::string date;
  };
  std::vector<LapRecord> topRecords;
  std::mutex recordsMutex;

  // AABB intersection check
  bool checkAABB(const SDL_FRect &a, const SDL_FRect &b) const;

  void sendLapTime(const std::string &playerName, float time,
                   const std::string &trackName, std::vector<uint8_t> ghostData);

  GhostManager* gm_ = nullptr;
};

#endif

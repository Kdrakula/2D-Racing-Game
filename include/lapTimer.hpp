#ifndef LAPTIMER_HPP
#define LAPTIMER_HPP

#include <SDL3/SDL.h>
#include <mutex>
#include <string>
#include <vector>

class LapTimer {
public:
  LapTimer();

  // Pass the player's bounding box and hit checkpoint status
  void update(const SDL_FRect &playerBox, float windowWidth, float windowHeight,
              const std::string &playerName);

  // Draw the finish line and checkpoints when debug is true
  void renderDebug(SDL_Renderer *renderer, const SDL_FRect &camera);

  // Render on-screen timer
  void renderTime(SDL_Renderer *renderer, float windowWidth);

  // Draw leaderboard overlay when user holds TAB
  void renderResults(SDL_Renderer *renderer, float windowWidth,
                     float windowHeight);

private:
  SDL_FRect finishLine;
  SDL_FRect checkpoint;
  bool hitCheckpoint;
  Uint64 lapStartTime;
  Uint64 bestLapTime;

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

  // Async leaderboard fetching
  void fetchLeaderboard();
};

#endif

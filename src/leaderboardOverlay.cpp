#include "leaderboardOverlay.hpp"
#include "gameConstants.hpp"

LeaderboardOverlay::LeaderboardOverlay(const bool &showResults,
                                       LapTimer &lapTimer)
    : showResults_(showResults), lapTimer_(lapTimer) {}

bool LeaderboardOverlay::isActive() const { return showResults_; }

void LeaderboardOverlay::render(SDL_Renderer *renderer, TTF_Font * /*font*/) {
  lapTimer_.renderResults(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
}

#include "lapTimer.hpp"
#include "engine.hpp" // For Game::font
#include "ghostManager.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp> // For JSON serialization
#include <vector>
#include <cstdlib>

#include "networkManager.hpp"
#include "utils.hpp"

LapTimer::LapTimer(GhostManager* gm) : gm_(gm) {
  startTime = SDL_GetTicks();
  lastLapTime = 0.0f;
  bestLapTime = 0.0f;
  isLapStarted = false;
  bestLapSet = false;
}

bool LapTimer::checkAABB(const SDL_FRect &a, const SDL_FRect &b) const {
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
          a.y + a.h > b.y);
}

void LapTimer::fetchLeaderboard(const std::string &trackName) {
  currentTrackName = trackName;
  std::thread([this, trackName]() {
    if (!NetworkManager::getInstance().isOnline()) {
        std::lock_guard<std::mutex> lock(recordsMutex);
        topRecords.clear();
        return;
    }

    httplib::Client cli(NetworkManager::getInstance().getActiveUrl());
    std::string url = "/api/laptimes?map_id=" + urlEncode(trackName);

    if (auto res = cli.Get(url.c_str())) {
      if (res->status == 200) {
        std::vector<LapRecord> newRecords;

        try {
          auto json = nlohmann::json::parse(res->body);
          bool isFirst = true;
          for (const auto &item : json) {
            LapRecord record;
            record.player = item.value("player", "Unknown");
            record.time = item.value("time", 0.0f);
            record.date = item.value("date", "");
            newRecords.push_back(record);

            // Sync world record ghost if available
            if (isFirst && gm_ && item.contains("ghost") && !item["ghost"].is_null()) {
                std::string b64 = item["ghost"];
                if (!b64.empty()) {
                    std::vector<uint8_t> ghostData = base64_decode(b64);
                    gm_->loadFromBuffer(ghostData);
                    gm_->setGhostPlayerName(record.player);
                }
            }
            isFirst = false;
          }
        } catch (const nlohmann::json::parse_error &e) {
          std::cerr << "[NETWORK] JSON parse error: " << e.what() << std::endl;
        }

        std::lock_guard<std::mutex> lock(recordsMutex);
        topRecords = newRecords;
      } else {
        std::cerr << "[NETWORK] Failed to fetch leaderboard. Status: "
                  << res->status << std::endl;
      }
    } else {
      std::cerr << "[NETWORK] Failed to reach leaderboard server. Error: "
                << httplib::to_string(res.error()) << std::endl;
    }
  }).detach();
}

void LapTimer::sendLapTime(const std::string &playerName, float time,
                         const std::string &trackName, std::vector<uint8_t> ghostData) {
  std::thread([this, playerName, time, trackName, ghostData]() {
    if (!NetworkManager::getInstance().isOnline()) {
        NetworkManager::getInstance().queueOfflineLap(playerName, trackName, time, ghostData);
        return;
    }

    httplib::Client cli(NetworkManager::getInstance().getActiveUrl());
    std::string url = "/api/laptime";

    nlohmann::json j;
    j["player"] = playerName;
    j["map_id"] = trackName;
    j["time"] = time;

    if (!ghostData.empty()) {
        j["ghost"] = base64_encode(ghostData);
    }

    std::string payload = j.dump();

    if (auto res = cli.Post(url.c_str(), payload, "application/json")) {
      if (res->status == 200) {
        std::cout << "\n[NETWORK] Successfully sent lap time to server! Status: "
                  << res->status << std::endl;
        this->fetchLeaderboard(trackName); // Refresh after upload
      } else {
        std::cerr << "\n[NETWORK] Server returned error status: " << res->status << std::endl;
        NetworkManager::getInstance().queueOfflineLap(playerName, trackName, time, ghostData);
      }
    } else {
      std::cerr << "\n[NETWORK] Failed to send lap time to server. Error: "
                << httplib::to_string(res.error()) << std::endl;
      NetworkManager::getInstance().queueOfflineLap(playerName, trackName, time, ghostData);
    }
  }).detach();
}

Uint32 LapTimer::getCurrentLapTimeMs() const {
    if (!isLapStarted) return 0;
    return SDL_GetTicks() - startTime;
}

int LapTimer::update(const SDL_FRect &playerBox, const TrackInfo &track, const std::string &playerName) {
  int status = 0; // running

  // Initialize track checkpoints if needed (or just use them from track)
  if (currentTrackName != track.name) {
    currentTrackName = track.name;
    checkpoints = track.checkpoints;
    finishLine = track.finishLine;
    hitCheckpoints.assign(checkpoints.size(), false);
    isLapStarted = false; // Reset lap state for new track
    fetchLeaderboard(track.name);
  }

  Uint64 currentTime = SDL_GetTicks();

  // Start lap when player crosses finish line for the first time or after a lap
  if (!isLapStarted && checkAABB(playerBox, finishLine)) {
    isLapStarted = true;
    startTime = currentTime;
    std::cout << "\n[LAP] Started!" << std::endl;
    status = 1;
  }

  if (!isLapStarted) {
    return status;
  }

  // Check checkpoints in order
  for (size_t i = 0; i < checkpoints.size(); ++i) {
    bool previousHit = (i == 0) ? true : hitCheckpoints[i - 1];
    if (previousHit && !hitCheckpoints[i] &&
        checkAABB(playerBox, checkpoints[i])) {
      hitCheckpoints[i] = true;
      std::cout << "\n[CHECKPOINT] " << (i + 1) << " / " << checkpoints.size()
                << " Reached!" << std::endl;
    }
  }

  // Check finish line
  bool allCheckpointsHit = true;
  for (bool hit : hitCheckpoints) {
    if (!hit) {
      allCheckpointsHit = false;
      break;
    }
  }

  if (allCheckpointsHit && checkAABB(playerBox, finishLine)) {
    for (size_t i = 0; i < hitCheckpoints.size(); ++i) {
      hitCheckpoints[i] = false;
    }
    isLapStarted = false;

    lastLapTime = (currentTime - startTime) / 1000.0f;
    std::cout << "\n[LAP FINISHED] Time: " << lastLapTime << "s" << std::endl;

    status = 2; // finished lap

    bool isNewRecord = (!bestLapSet || lastLapTime < bestLapTime);
    if (isNewRecord) {
      bestLapTime = lastLapTime;
      bestLapSet = true;
      std::cout << ">>> NEW BEST LAP: " << bestLapTime << "s <<<" << std::endl;
      
      status = 3; // new best

      if (gm_) {
          gm_->saveBestLap(track.name);
      }
    }

    std::vector<uint8_t> ghostPayload;
    if (isNewRecord && gm_) {
        ghostPayload = gm_->getSerializedBestLap();
    }
    
    sendLapTime(playerName, lastLapTime, track.name, ghostPayload);
  }

  return status;
}

void LapTimer::renderResults(SDL_Renderer *renderer, float windowWidth,
                             float windowHeight) {
  // Semi-transparent black background
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);

  float boxWidth = 550.0f;
  float boxHeight = 500.0f;
  SDL_FRect bgRect = {(windowWidth - boxWidth) / 2.0f,
                      (windowHeight - boxHeight) / 2.0f, boxWidth, boxHeight};
  SDL_RenderFillRect(renderer, &bgRect);

  // Outline
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderRect(renderer, &bgRect);

  SDL_Color textColor = {255, 255, 255, 255};

  // Title render using TTF
  if (Game::font) {
    SDL_Surface *titleSurface = TTF_RenderText_Blended(
        Game::font, "--- TOP LAP TIMES ---", 0, textColor);
    if (titleSurface) {
      SDL_Texture *titleTexture =
          SDL_CreateTextureFromSurface(renderer, titleSurface);
      if (titleTexture) {
        SDL_FRect titleRect = {bgRect.x + (boxWidth - titleSurface->w) / 2.0f,
                               bgRect.y + 20.0f, (float)titleSurface->w,
                               (float)titleSurface->h};
        SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
        SDL_DestroyTexture(titleTexture);
      }
      SDL_DestroySurface(titleSurface);
    }
  }

  // Lock and draw records
  std::lock_guard<std::mutex> lock(recordsMutex);

  if (topRecords.empty()) {
    if (Game::font) {
      SDL_Surface *txtSurface =
          TTF_RenderText_Blended(Game::font, "No records found.", 0, textColor);
      if (txtSurface) {
        SDL_Texture *txtTex =
            SDL_CreateTextureFromSurface(renderer, txtSurface);
        if (txtTex) {
          SDL_FRect textRect = {bgRect.x + 120.0f, bgRect.y + 80.0f,
                                (float)txtSurface->w, (float)txtSurface->h};
          SDL_RenderTexture(renderer, txtTex, nullptr, &textRect);
          SDL_DestroyTexture(txtTex);
        }
        SDL_DestroySurface(txtSurface);
      }
    }
  } else {
    float startY = bgRect.y + 80;
    for (size_t i = 0; i < topRecords.size(); i++) {
      std::stringstream ss;
      ss << (i + 1) << ". " << topRecords[i].player
         << " ==== " << topRecords[i].time << "s";
      // if (!topRecords[i].date.empty())
      //    ss << " (" << topRecords[i].date << ")";

      // Render text using TTF
      if (Game::font) {
        SDL_Surface *rowSurface =
            TTF_RenderText_Blended(Game::font, ss.str().c_str(), 0, textColor);
        if (rowSurface) {
          SDL_Texture *rowTexture =
              SDL_CreateTextureFromSurface(renderer, rowSurface);
          if (rowTexture) {
            SDL_FRect rowRect = {bgRect.x + 40.0f, startY + (i * 35.0f),
                                 (float)rowSurface->w, (float)rowSurface->h};
            SDL_RenderTexture(renderer, rowTexture, nullptr, &rowRect);
            SDL_DestroyTexture(rowTexture);
          }
          SDL_DestroySurface(rowSurface);
        }
      }
    }
  }

  // Draw Network Status
  if (Game::font) {
    std::string netStatus = "Network: " + NetworkManager::getInstance().getStatusString();
    SDL_Color netColor = NetworkManager::getInstance().isOnline() ? SDL_Color{0, 255, 0, 255} : SDL_Color{255, 0, 0, 255};
    SDL_Surface *netSurface = TTF_RenderText_Blended(Game::font, netStatus.c_str(), 0, netColor);
    if (netSurface) {
      SDL_Texture *netTexture = SDL_CreateTextureFromSurface(renderer, netSurface);
      if (netTexture) {
        SDL_FRect netRect = {bgRect.x + 20.0f,
                             bgRect.y + boxHeight - netSurface->h - 20.0f, 
                             (float)netSurface->w,
                             (float)netSurface->h};
        SDL_RenderTexture(renderer, netTexture, nullptr, &netRect);
        SDL_DestroyTexture(netTexture);
      }
      SDL_DestroySurface(netSurface);
    }
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void LapTimer::render(SDL_Renderer *renderer, const SDL_FRect &camera, bool debug,
                       const TrackInfo &track) {
  if (!debug)
    return;

  // Enable alpha blending for semi-transparent boxes
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  // Draw Finish Line (Red, Semi-transparent fill)
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
  SDL_FRect renderFinish = {track.finishLine.x - camera.x,
                            track.finishLine.y - camera.y, track.finishLine.w,
                            track.finishLine.h};
  SDL_RenderFillRect(renderer, &renderFinish);

  // Draw Checkpoints
  for (size_t i = 0; i < track.checkpoints.size(); ++i) {
    bool hit = (i < hitCheckpoints.size()) ? hitCheckpoints[i] : false;
    if (hit) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100); // Green if hit
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100); // Yellow if not hit
    }
    SDL_FRect renderCP = {track.checkpoints[i].x - camera.x,
                          track.checkpoints[i].y - camera.y,
                          track.checkpoints[i].w, track.checkpoints[i].h};
    SDL_RenderFillRect(renderer, &renderCP);
  }
  // Disable alpha blending to return to normal rendering
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void LapTimer::renderTime(SDL_Renderer *renderer, float windowWidth) {
  float timeInSeconds = 0.0f;
  if (isLapStarted) {
      Uint64 currentLapTime = SDL_GetTicks() - startTime;
      timeInSeconds = currentLapTime / 1000.0f;
  }

  std::stringstream ss;
  ss.precision(3);
  ss << std::fixed << "Lap Time: " << timeInSeconds << " s";

  // Slightly transparent black background for text
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
  SDL_FRect bgRect = {(windowWidth - 250.0f) / 2.0f, 10.0f, 250.0f, 50.0f};
  SDL_RenderFillRect(renderer, &bgRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  if (Game::font) {
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *timeSurface =
        TTF_RenderText_Blended(Game::font, ss.str().c_str(), 0, textColor);
    if (timeSurface) {
      SDL_Texture *timeTexture =
          SDL_CreateTextureFromSurface(renderer, timeSurface);
      if (timeTexture) {
        SDL_FRect timeRect = {bgRect.x + (bgRect.w - timeSurface->w) / 2.0f,
                              bgRect.y + (bgRect.h - timeSurface->h) / 2.0f,
                              (float)timeSurface->w, (float)timeSurface->h};
        SDL_RenderTexture(renderer, timeTexture, nullptr, &timeRect);
        SDL_DestroyTexture(timeTexture);
      }
      SDL_DestroySurface(timeSurface);
    }
  }
}

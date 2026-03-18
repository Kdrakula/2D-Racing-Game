#include "lapTimer.hpp"
#include "engine.hpp" // For Game::font
#include <SDL3_ttf/SDL_ttf.h>
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

LapTimer::LapTimer() {
  finishLine = {300.0f, 400.0f, 200.0f, 50.0f};
  checkpoint = {2360.0f, 1600.0f, 200.0f, 50.0f};

  hitCheckpoint = false;
  lapStartTime = SDL_GetTicks();
  bestLapTime = 0;

  // Fetch immediately on startup
  fetchLeaderboard();
}

bool LapTimer::checkAABB(const SDL_FRect &a, const SDL_FRect &b) const {
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
          a.y + a.h > b.y);
}

void LapTimer::fetchLeaderboard() {
  std::thread([this]() {
    httplib::Client cli(
        "http://127.0.0.1:4000"); // Use IPv4 loopback directly to avoid IPv6
                                  // resolution issues

    if (auto res = cli.Get("/api/laptimes")) {
      if (res->status == 200) {
        std::vector<LapRecord> newRecords;

        // Very basic JSON parser for the expected array format
        // e.g. [{"player": "Kuba", "time": 12.5}, ...]
        std::string body = res->body;
        size_t pos = 0;

        while ((pos = body.find("\"player\"", pos)) != std::string::npos) {
          LapRecord record;

          // Extract player name
          size_t playerStart = body.find("\"", pos + 8);
          if (playerStart == std::string::npos)
            break;
          playerStart += 1;
          size_t playerEnd = body.find("\"", playerStart);
          if (playerEnd != std::string::npos) {
            record.player = body.substr(playerStart, playerEnd - playerStart);
          }

          // Extract time
          size_t timePos = body.find("\"time\"", playerEnd);
          if (timePos == std::string::npos)
            break;

          size_t colonPos = body.find(":", timePos);
          if (colonPos == std::string::npos)
            break;

          size_t timeStart = colonPos + 1;
          while (timeStart < body.length() && std::isspace(body[timeStart])) {
            timeStart++;
          }

          size_t timeEnd = timeStart;
          while (timeEnd < body.length() &&
                 (std::isdigit(body[timeEnd]) || body[timeEnd] == '.')) {
            timeEnd++;
          }

          if (timeStart < timeEnd) {
            try {
              record.time =
                  std::stof(body.substr(timeStart, timeEnd - timeStart));
            } catch (...) {
              record.time = 0.0f;
            }
          }

          // Extract date
          size_t datePos = body.find("\"date\"", timeEnd);
          if (datePos != std::string::npos &&
              datePos < body.find("}", timeEnd)) {
            size_t dateStart = body.find("\"", datePos + 6);
            if (dateStart != std::string::npos) {
              dateStart += 1;
              size_t dateEnd = body.find("\"", dateStart);
              if (dateEnd != std::string::npos) {
                record.date = body.substr(dateStart, dateEnd - dateStart);
                pos = dateEnd;
              } else {
                pos = timeEnd;
              }
            } else {
              pos = timeEnd;
            }
          } else {
            pos = timeEnd;
          }

          newRecords.push_back(record);
        }

        std::lock_guard<std::mutex> lock(recordsMutex);
        topRecords = newRecords;
      }
    }
  }).detach();
}

void LapTimer::update(const SDL_FRect &playerBox, float windowWidth,
                      float windowHeight, const std::string &playerName) {

  Uint64 currentTime = SDL_GetTicks();

  if (!hitCheckpoint && checkAABB(playerBox, checkpoint)) {
    hitCheckpoint = true;
    std::cout << "\n[CHECKPOINT] Reached!" << std::endl;
  }

  if (hitCheckpoint && checkAABB(playerBox, finishLine)) {
    hitCheckpoint = false;
    Uint64 currentLapTime = currentTime - lapStartTime;
    float timeInSeconds = currentLapTime / 1000.0f;

    std::cout << "\n[LAP FINISHED] Time: " << timeInSeconds << "s" << std::endl;

    if (bestLapTime == 0 || currentLapTime < bestLapTime) {
      bestLapTime = currentLapTime;
      std::cout << ">>> NEW BEST LAP: " << timeInSeconds << "s <<<"
                << std::endl;

      // Send to backend on a background thread
      std::thread([this, timeInSeconds, playerName]() {
        httplib::Client cli(
            "http://127.0.0.1:4000"); // Updated port and proxy info

        std::string payload = "{\"player\": \"" + playerName +
                              "\", \"time\": " + std::to_string(timeInSeconds) +
                              "}";

        if (auto res = cli.Post("/api/laptime", payload, "application/json")) {
          std::cout
              << "\n[NETWORK] Successfully sent best lap to server! Status: "
              << res->status << std::endl;
        } else {
          std::cout << "\n[NETWORK] Failed to reach the server. Error: "
                    << to_string(res.error()) << std::endl;
        }

        // Fetch leaderboard again as it has been updated
        fetchLeaderboard();
      }).detach();
    }

    lapStartTime = SDL_GetTicks();
  }
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

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void LapTimer::renderDebug(SDL_Renderer *renderer, const SDL_FRect &camera) {
  // Enable alpha blending for semi-transparent boxes
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  // Draw Finish Line (Blue, Semi-transparent fill)
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100);
  SDL_FRect renderFinish = {finishLine.x - camera.x, finishLine.y - camera.y,
                            finishLine.w, finishLine.h};
  SDL_RenderFillRect(renderer, &renderFinish);

  // Draw Checkpoint (Yellow, Semi-transparent fill)
  SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
  SDL_FRect renderCheckpoint = {checkpoint.x - camera.x,
                                checkpoint.y - camera.y, checkpoint.w,
                                checkpoint.h};
  SDL_RenderFillRect(renderer, &renderCheckpoint);

  // Disable alpha blending to return to normal rendering
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void LapTimer::renderTime(SDL_Renderer *renderer, float windowWidth) {
  Uint64 currentLapTime = SDL_GetTicks() - lapStartTime;
  float timeInSeconds = currentLapTime / 1000.0f;

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

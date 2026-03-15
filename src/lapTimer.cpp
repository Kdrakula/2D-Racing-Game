#include "lapTimer.hpp"
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
        "http://localhost:4000"); // Updated port based on server.py

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
        httplib::Client cli("http://localhost:4000"); // Updated port

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

  // Title
  SDL_RenderDebugText(renderer, bgRect.x + 120, bgRect.y + 20,
                      "--- TOP LAP TIMES ---");

  // Lock and draw records
  std::lock_guard<std::mutex> lock(recordsMutex);

  if (topRecords.empty()) {
    SDL_RenderDebugText(renderer, bgRect.x + 120, bgRect.y + 60,
                        "No records found.");
  } else {
    float startY = bgRect.y + 60;
    for (size_t i = 0; i < topRecords.size(); i++) {
      std::stringstream ss;
      ss << (i + 1) << ". " << topRecords[i].player << " - "
         << topRecords[i].time << "s";
      if (!topRecords[i].date.empty())
        ss << " (" << topRecords[i].date << ")";

      // Render text
      SDL_RenderDebugText(renderer, bgRect.x + 40, startY + (i * 30),
                          ss.str().c_str());
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
  SDL_FRect bgRect = {(windowWidth - 200.0f) / 2.0f, 10.0f, 200.0f, 40.0f};
  SDL_RenderFillRect(renderer, &bgRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  SDL_RenderDebugText(renderer, bgRect.x + 20, bgRect.y + 10, ss.str().c_str());
}

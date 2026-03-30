#include "engine.hpp"
#include "debugOverlay.hpp"
#include "errorLoger.hpp"
#include "gameConstants.hpp"
#include "gameObject.hpp"
#include "leaderboardOverlay.hpp"
#include "nicknameOverlay.hpp"
#include "textureManager.hpp"
#include "updaterOverlay.hpp"
#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>


SDL_Renderer *Game::renderer = nullptr;
SDL_FRect Game::camera = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
TTF_Font *Game::font = nullptr;

void Game::checkForUpdates() {
  std::thread([this]() {
    httplib::Client cli("https://api.github.com");
    cli.set_follow_location(true);

    // GitHub API requires a User-Agent header
    httplib::Headers headers = {{"User-Agent", "MyRacingGame-AutoUpdater"}};

    if (auto res = cli.Get("/repos/Kdrakula/2D-Racing-Game/releases/latest",
                           headers)) {
      if (res->status == 200) {
        try {
          auto json = nlohmann::json::parse(res->body);
          std::string latestVersion = json["tag_name"];

          if (latestVersion != GAME_VERSION) {
            std::cout << "[UPDATER] New version found: " << latestVersion
                      << " (Current: " << GAME_VERSION << ")" << std::endl;

            // Find the macOS zip asset
            if (json.contains("assets") && json["assets"].is_array()) {
              for (const auto &asset : json["assets"]) {
                std::string assetName = asset["name"];
                if (assetName == "MyRacingGame-macOS.zip") {
                  this->updateDownloadUrl = asset["browser_download_url"];
                  this->updateAvailable = true;
                  break;
                }
              }
            }
          } else {
            std::cout << "[UPDATER] Game is up to date." << std::endl;
          }
        } catch (const std::exception &e) {
          std::cout << "[UPDATER] JSON Parse Error: " << e.what() << std::endl;
        }
      } else {
        std::cout << "[UPDATER] HTTP Error: " << res->status << std::endl;
      }
    } else {
      std::cout << "[UPDATER] Connection Failed: " << to_string(res.error())
                << std::endl;
    }
  }).detach();
}

Game::Game(const char *title, int width, int height, const TrackInfo &track)
    : track_(track) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    logSDLError(std::cout, "Init");
    isRunning = false;
    return;
  }

  if (!TTF_Init()) {
    std::cout << "TTF_Init Error: " << SDL_GetError() << std::endl;
    isRunning = false;
    return;
  }

  window = SDL_CreateWindow(title, width, height, 0);
  if (window == nullptr)
    logSDLError(std::cout, "Create window");

  renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr)
    logSDLError(std::cout, "Create renderer");

  const char *fontBasePath = SDL_GetBasePath();
  std::string fontPathStr =
      std::string(fontBasePath) + "assets/Roboto-Regular.ttf";

  // Quick fix for macOS if basePath points inside .app/Contents/MacOS/
  size_t fontPos = fontPathStr.rfind(".app/Contents/MacOS/");
  if (fontPos != std::string::npos) {
    fontPathStr = fontPathStr.substr(0, fontPos) +
                  ".app/Contents/Resources/assets/Roboto-Regular.ttf";
  }

  font = TTF_OpenFont(fontPathStr.c_str(), 24);
  if (font == nullptr) {
    std::cout << "Failed to load font from " << fontPathStr << ": "
              << SDL_GetError() << std::endl;
  }

  // Load visual background from track
  bg = TextureManager::loadTexture(track.bgAsset.c_str());

  // Load hitmap mask from track
  if (!collision.loadMask(track.maskAsset.c_str())) {
    logSDLError(std::cout, "Load Collision Mask");
  } else {
    // Create a texture for visualization
    maskTexture = SDL_CreateTextureFromSurface(renderer, collision.getSurface());
    if (maskTexture) {
      SDL_SetTextureBlendMode(maskTexture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(maskTexture, 128); // 50% transparency
    }
  }

  player = new GameObject("assets/car.png", track.startX, track.startY);

  // Initialise live player bounding box (kept in sync in update())
  playerBox_ = {track.startX, track.startY, 32.0f, 64.0f};

  // Build overlay stack (render order = push order)
  auto dbg = std::make_unique<DebugOverlay>(isDebugMode, playerBox_, camera,
                                            lapTimer, player->vel,
                                            player->angle, GAME_VERSION);
  debugOverlay_ = dbg.get();
  overlays.push_back(std::move(dbg));
  overlays.push_back(
      std::make_unique<NicknameOverlay>(input.isTypingName, input.playerName));
  overlays.push_back(
      std::make_unique<LeaderboardOverlay>(input.showResults, lapTimer));
  overlays.push_back(std::make_unique<UpdaterOverlay>(updateAvailable));

  isRunning = true;
  checkForUpdates();
}

Game::~Game() = default;

void Game::handleEvents() {
  input.update();

  if (input.isQuitRequested) {
    isRunning = false;
  }
  if (input.isDebugToggled) {
    isDebugMode = !isDebugMode;
    std::cout << "\n[DEVELOPER MODE] " << (isDebugMode ? "ON" : "OFF")
              << std::endl;
  }
  if (input.showMaskToggled) {
    input.showMask = !input.showMask;
    std::cout << "\n[MASK VISUALIZATION] " << (input.showMask ? "ON" : "OFF")
              << std::endl;
  }
}

void Game::update() {

  // 1. Pass steering directly to the player
  player->dir = input.steeringDir;

  // This guarantees we never get stuck in a wall
  float oldX = player->posx;
  float oldY = player->posy;

  // --- 2. Move Player ---
  player->update();

  // --- 3. Outer Map Boundaries ---
  // Prevent the car from leaving the map dimensions entirely
  bool hitBoundary = false;
  if (player->posx < 0.0f) {
    player->posx = 0.0f;
    hitBoundary = true;
  }
  if (player->posy < 0.0f) {
    player->posy = 0.0f;
    hitBoundary = true;
  }
  if (player->posx + 32.0f > MAP_WIDTH) {
    player->posx = MAP_WIDTH - 32.0f;
    hitBoundary = true;
  }
  if (player->posy + 64.0f > MAP_HEIGHT) {
    player->posy = MAP_HEIGHT - 64.0f;
    hitBoundary = true;
  }

  if (hitBoundary) {
    player->vel *= -0.5f; // Bounce off the map edge
  }

  // --- 4. Mask Collision Check ---
  if (collision.checkMaskCollision(player->posx, player->posy, 32.0f, 64.0f)) {
    // The ultimate fix: instantly revert to the safe position
    player->posx = oldX;
    player->posy = oldY;

    // Bounce the car back
    player->vel *= -0.5f;
  }

  // --- 5. Update Camera ---
  // Center camera perfectly on the 32x64 car
  camera.x = player->posx + 16.0f - (WINDOW_WIDTH / 2.0f);
  camera.y = player->posy + 32.0f - (WINDOW_HEIGHT / 2.0f);

  // Clamp camera within map bounds
  if (camera.x < 0)
    camera.x = 0;
  if (camera.x > MAP_WIDTH - camera.w)
    camera.x = MAP_WIDTH - camera.w;
  if (camera.y < 0)
    camera.y = 0;
  if (camera.y > MAP_HEIGHT - camera.h)
    camera.y = MAP_HEIGHT - camera.h;

  // --- 6. Lap Timing Logic ---
  playerBox_ = {player->posx, player->posy, 32.0f, 64.0f};
  lapTimer.update(playerBox_, track_);

  // 7. Physics Constraints using InputManager
  if (input.up && player->vel < TOP_SPEED)
    player->vel += ACCELERATION;
  if (input.down && player->vel > -TOP_SPEED)
    player->vel -= ACCELERATION;

  if (input.braking && player->vel > 0)
    player->vel -= BRAKE_FORCE * DEACCELERATION;
  if (input.braking && player->vel < 0)
    player->vel += BRAKE_FORCE * DEACCELERATION;

  if (!input.up && player->vel > 0)
    player->vel -= DEACCELERATION;
  if (!input.down && player->vel < 0)
    player->vel += DEACCELERATION;

  if (std::abs(player->vel) < 0.02f)
    player->vel = 0.0f;

  // --- AUTO UPDATER EXECUTION ---
  if (updateAvailable && input.updateKey) {
    std::cout << "[UPDATER] Starting update sequence..." << std::endl;

    // Determine paths securely inside main loop
    const char *basePath = SDL_GetBasePath();
    std::string currentAppPath = std::string(basePath);

    // Remove /Contents/Resources/ assuming standard macOS structure
    size_t pos = currentAppPath.rfind(".app/");
    if (pos != std::string::npos) {
      currentAppPath = currentAppPath.substr(0, pos + 4);
    }

    std::string scriptPath =
        currentAppPath + "/Contents/Resources/assets/updater.sh";

    // Execute the bash script in the background using ampersand
    std::string cmd = "\"" + scriptPath + "\" " + updateDownloadUrl + " \"" +
                      currentAppPath + "\" &";
    std::cout << "[UPDATER] Running: " << cmd << std::endl;
    system(cmd.c_str());

    // Let the main thread exit natively so macOS releases the app handle
    // gracefully
    isRunning = false;
  }

  // --- 8. DEBUG: Live Telemetry ---
  if (isDebugMode && debugOverlay_) {
    debugOverlay_->renderTelemetry();
  }
}

void Game::render() {
  SDL_RenderClear(renderer);

  // Render the background relative to the camera
  SDL_RenderTexture(renderer, bg, &camera, nullptr);

  // Render the collision mask if requested
  if (input.showMask && maskTexture) {
    SDL_RenderTexture(renderer, maskTexture, &camera, nullptr);
  }

  // Render the player texture
  player->render();

  lapTimer.renderTime(renderer, WINDOW_WIDTH);
  lapTimer.render(renderer, camera, isDebugMode, track_);

  // --- OVERLAYS (each checks its own active condition) ---
  for (auto &o : overlays) {
    if (o->isActive())
      o->render(renderer, font);
  }

  // Reset the renderer color back to black for the next frame
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_RenderPresent(renderer);
}

void Game::clean() {
  delete player;
  SDL_DestroyTexture(bg);
  if (maskTexture) {
    SDL_DestroyTexture(maskTexture);
  }

  // Mask is freed automatically in CollisionManager's destructor

  if (font) {
    TTF_CloseFont(font);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  std::cout << "Engine cleaned successfully." << std::endl;
}

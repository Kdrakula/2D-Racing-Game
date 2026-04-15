#include "menu.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Helper: resolve an asset path for both dev build and macOS .app bundle
// ---------------------------------------------------------------------------
std::string Menu::resolveAsset(const std::string &relative) const {
  std::string base = SDL_GetBasePath();
  std::string full = base + relative;
  size_t pos = full.rfind(".app/Contents/MacOS/");
  if (pos != std::string::npos) {
    full = full.substr(0, pos) + ".app/Contents/Resources/" + relative;
  }
  return full;
}

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
Menu::Menu() {
  tracks_ = {{"Test Void",
              "assets/track-1.png",
              "assets/maks-1.png",
              393.0f,
              364.0f, // starting posistion
              0.,
              {2360.0f, 1500.0f, 200.0f, 50.0f}, // finishLine
              {{2360.0f, 1600.0f, 200.0f, 50.0f},
               {4000.0f, 2000.0f, 200.0f, 50.0f}}}, // checkpoints
             {"F1 Race Track",
              "assets/track-2.png",
              "assets/mask-2.png",
              1950.0f,
              1680.0f,
              0.,
              {1900.0f, 1600.0f, 20.0f, 200.0f}, // finishLine
              {{1900.0f, 1280.0f, 20.0f, 200.0f},
               {1900.0f, 930.0f, 20.0f, 200.0f}}}, // checkpoints
             {"Neon City Night",
              "assets/track-neon.png",
              "assets/mask-neon.png",
              100.0f,
              100.0f,
              0.,
              {100, 100, 50, 200},
              {}},
             {"Snowy Peaks",
              "assets/track-snow.png",
              "assets/mask-snow.png",
              100.0f,
              100.0f,
              0.,
              {100, 100, 50, 200},
              {}},
             {"Retro Arcade",
              "assets/track-retro.png",
              "assets/mask-retro.png",
              100.0f,
              100.0f,
              0.,
              {100, 100, 50, 200},
              {}},
             {"Forest Trail",
              "assets/track-forest.png",
              "assets/mask-forest.png",
              100.0f,
              100.0f,
              0.,
              {100, 100, 50, 200},
              {}}};
} // todo all tracks should contain their whole information in one file
// todo add map generator

Menu::~Menu() {}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
bool Menu::run() {
  init();
  while (running_) {
    handleEvents();
    render();
    SDL_Delay(16); // ~60 fps
    // todo correct FPS handling
    // todo ping to server
  }
  clean();
  return confirmed_;
}

TrackInfo Menu::selectedTrack() const { return tracks_[selectedIndex_]; }

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
void Menu::init() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cout << "[MENU] SDL_Init failed: " << SDL_GetError() << std::endl;
    return;
  }
  if (!TTF_Init()) {
    std::cout << "[MENU] TTF_Init failed: " << SDL_GetError() << std::endl;
    return;
  }

  window_ = SDL_CreateWindow("My Racing Game – Select Track",
                             static_cast<int>(WINDOW_WIDTH),
                             static_cast<int>(WINDOW_HEIGHT), 0);
  renderer_ = SDL_CreateRenderer(window_, nullptr);

  std::string fontPath = resolveAsset("assets/Roboto-Regular.ttf");
  titleFont_ = TTF_OpenFont(fontPath.c_str(), 52);
  bodyFont_ = TTF_OpenFont(fontPath.c_str(), 22);

  if (!titleFont_ || !bodyFont_) {
    std::cout << "[MENU] Font load failed: " << SDL_GetError() << std::endl;
  }

  // Load track thumbnails
  for (auto &track : tracks_) {
    std::string imgPath = resolveAsset(track.bgAsset);
    SDL_Surface *surf = IMG_Load(imgPath.c_str());
    if (surf) {
      thumbnails_.push_back(SDL_CreateTextureFromSurface(renderer_, surf));
      SDL_DestroySurface(surf);
    } else {
      thumbnails_.push_back(nullptr);
    }
  }

  running_ = true;
}

// ---------------------------------------------------------------------------
// handleEvents
// ---------------------------------------------------------------------------
void Menu::handleEvents() { // todo menu shouldn't be another window
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT) {
      running_ = false;
      confirmed_ = false;
    }
    if (e.type == SDL_EVENT_KEY_DOWN) {
      switch (e.key.scancode) {
      case SDL_SCANCODE_LEFT:
      case SDL_SCANCODE_UP:
        selectedIndex_ =
            (selectedIndex_ - 1 + static_cast<int>(tracks_.size())) %
            static_cast<int>(tracks_.size());
        break;
      case SDL_SCANCODE_RIGHT:
      case SDL_SCANCODE_DOWN:
        selectedIndex_ =
            (selectedIndex_ + 1) % static_cast<int>(tracks_.size());
        break;
      case SDL_SCANCODE_RETURN:
      case SDL_SCANCODE_KP_ENTER:
        confirmed_ = true;
        running_ = false;
        break;
      case SDL_SCANCODE_ESCAPE:
        confirmed_ = false;
        running_ = false;
        break;
      default:
        break;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Helpers: render a single line of TTF text
// ---------------------------------------------------------------------------
static void renderText(SDL_Renderer *r, TTF_Font *font, const std::string &text,
                       SDL_Color color, float x, float y) {
  if (!font)
    return;
  SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), 0, color);
  if (!surf)
    return;
  SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
  if (tex) {
    float tw = 0, th = 0;
    SDL_GetTextureSize(tex, &tw, &th);
    SDL_FRect dst = {x, y, tw, th};
    SDL_RenderTexture(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
  }
  SDL_DestroySurface(surf);
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------
void Menu::render() {
  // --- Background gradient (top-dark to bottom-slightly-lighter) ---
  SDL_SetRenderDrawColor(renderer_, 10, 10, 22, 255);
  SDL_RenderClear(renderer_);

  // Subtle horizontal gradient bands
  for (int i = 0; i < static_cast<int>(WINDOW_HEIGHT); ++i) {
    Uint8 v =
        static_cast<Uint8>(10 + (i * 18) / static_cast<int>(WINDOW_HEIGHT));
    SDL_SetRenderDrawColor(renderer_, v / 2, v / 2, v, 255);
    SDL_FRect line = {0, static_cast<float>(i), WINDOW_WIDTH, 1};
    SDL_RenderFillRect(renderer_, &line);
  }

  // --- Title ---
  SDL_Color gold = {255, 210, 50, 255};
  SDL_Color white = {230, 230, 230, 255};
  SDL_Color grey = {140, 140, 160, 255};

  // Centre title horizontally
  if (titleFont_) {
    const std::string title = "MY RACING GAME";
    SDL_Surface *ts =
        TTF_RenderText_Blended(titleFont_, title.c_str(), 0, gold);
    if (ts) {
      SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer_, ts);
      if (tt) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tt, &tw, &th);
        SDL_FRect dst = {(WINDOW_WIDTH - tw) / 2.0f, 40.0f, tw, th};
        SDL_RenderTexture(renderer_, tt, nullptr, &dst);
        SDL_DestroyTexture(tt);
      }
      SDL_DestroySurface(ts);
    }
  }

  renderText(renderer_, bodyFont_, "SELECT A TRACK", grey,
             (WINDOW_WIDTH - 200.0f) / 2.0f, 110.0f);

  // --- Track cards ---
  const float CARD_W = 340.0f;
  const float CARD_H = 240.0f; // thumbnail (190px) + name bar (50px)
  const float THUMB_H = 190.0f;
  const float GAP = 40.0f;
  const int n = static_cast<int>(tracks_.size());

  // Update continuous index for smooth scrolling
  float diff = selectedIndex_ - continuousIndex_;
  if (diff > n / 2.0f)
    diff -= n;
  else if (diff < -n / 2.0f)
    diff += n;
  continuousIndex_ += diff * 0.12f;
  if (continuousIndex_ < 0)
    continuousIndex_ += n;
  if (continuousIndex_ >= n)
    continuousIndex_ -= n;

  float centerX = (WINDOW_WIDTH - CARD_W) / 2.0f;
  float cardY = 155.0f;

  for (int i = 0; i < n; ++i) {
    float itemDiff = i - continuousIndex_;
    if (itemDiff > n / 2.0f)
      itemDiff -= n;
    else if (itemDiff < -n / 2.0f)
      itemDiff += n;

    float cx = centerX + itemDiff * (CARD_W + GAP);

    // Don't render cards that are completely off screen
    if (cx + CARD_W < -100 || cx > WINDOW_WIDTH + 100)
      continue;

    bool selected = (i == selectedIndex_);

    // Calculate opacity based on distance from center
    float distance = std::abs(itemDiff);
    Uint8 alpha = 255;
    if (distance > 0.0f) {
      float a = 255.0f - (distance * 130.0f);
      if (a < 30.0f)
        a = 30.0f;
      alpha = static_cast<Uint8>(a);
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Outer glow / border
    if (selected) {
      SDL_SetRenderDrawColor(renderer_, 255, 210, 50, alpha); // gold highlight
      SDL_FRect border = {cx - 3, cardY - 3, CARD_W + 6, CARD_H + 6};
      SDL_RenderFillRect(renderer_, &border);
    } else {
      SDL_SetRenderDrawColor(renderer_, 60, 60, 80, alpha);
      SDL_FRect border = {cx - 1, cardY - 1, CARD_W + 2, CARD_H + 2};
      SDL_RenderFillRect(renderer_, &border);
    }

    // Card background
    SDL_SetRenderDrawColor(renderer_, 20, 20, 38, alpha);
    SDL_FRect card = {cx, cardY, CARD_W, CARD_H};
    SDL_RenderFillRect(renderer_, &card);

    // Thumbnail
    SDL_FRect thumbDst = {cx, cardY, CARD_W, THUMB_H};
    if (thumbnails_[i]) {
      SDL_SetTextureBlendMode(thumbnails_[i], SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(thumbnails_[i], alpha);
      SDL_RenderTexture(renderer_, thumbnails_[i], nullptr, &thumbDst);
      SDL_SetTextureAlphaMod(thumbnails_[i], 255); // restore
    } else {
      // Fallback: solid colour placeholder
      SDL_SetRenderDrawColor(renderer_, 40, 80, 60, alpha);
      SDL_RenderFillRect(renderer_, &thumbDst);
      SDL_Color fallbackColor = grey;
      fallbackColor.a = alpha;
      renderText(renderer_, bodyFont_, "[ No Preview ]", fallbackColor,
                 cx + 100.0f, cardY + THUMB_H / 2.0f - 11.0f);
    }

    // Name bar background
    SDL_FRect nameBar = {cx, cardY + THUMB_H, CARD_W, CARD_H - THUMB_H};
    SDL_SetRenderDrawColor(renderer_, selected ? 40 : 25, selected ? 35 : 22,
                           selected ? 60 : 42, alpha);
    SDL_RenderFillRect(renderer_, &nameBar);

    // Track name
    SDL_Color nameColor = selected ? gold : white;
    if (bodyFont_) {
      SDL_Surface *ns = TTF_RenderText_Blended(
          bodyFont_, tracks_[i].name.c_str(), 0, nameColor);
      if (ns) {
        SDL_Texture *nt = SDL_CreateTextureFromSurface(renderer_, ns);
        if (nt) {
          SDL_SetTextureBlendMode(nt, SDL_BLENDMODE_BLEND);
          SDL_SetTextureAlphaMod(nt, alpha);
          float tw = 0, th = 0;
          SDL_GetTextureSize(nt, &tw, &th);
          SDL_FRect dst = {cx + (CARD_W - tw) / 2.0f,
                           cardY + THUMB_H + (CARD_H - THUMB_H - th) / 2.0f, tw,
                           th};
          SDL_RenderTexture(renderer_, nt, nullptr, &dst);
          SDL_DestroyTexture(nt);
        }
        SDL_DestroySurface(ns);
      }
    }
  }

  // // --- Bottom instructions ---
  // renderText(renderer_, bodyFont_,
  //            " <- ->  to select     ENTER to race     ESC to quit", grey, 0,
  //            WINDOW_HEIGHT - 40.0f);
  // Centre the hint
  if (bodyFont_) {
    const std::string hint =
        "<- ->  to select     ENTER to race     ESC to quit";
    SDL_Surface *hs = TTF_RenderText_Blended(bodyFont_, hint.c_str(), 0, grey);
    if (hs) {
      SDL_Texture *ht = SDL_CreateTextureFromSurface(renderer_, hs);
      if (ht) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(ht, &tw, &th);
        SDL_FRect dst = {(WINDOW_WIDTH - tw) / 2.0f, WINDOW_HEIGHT - 46.0f, tw,
                         th};
        SDL_RenderTexture(renderer_, ht, nullptr, &dst);
        SDL_DestroyTexture(ht);
      }
      SDL_DestroySurface(hs);
    }
  }

  SDL_RenderPresent(renderer_);
}

// ---------------------------------------------------------------------------
// clean
// ---------------------------------------------------------------------------
void Menu::clean() {
  for (auto *t : thumbnails_) {
    if (t)
      SDL_DestroyTexture(t);
  }
  thumbnails_.clear();

  if (titleFont_)
    TTF_CloseFont(titleFont_);
  if (bodyFont_)
    TTF_CloseFont(bodyFont_);
  if (renderer_)
    SDL_DestroyRenderer(renderer_);
  if (window_)
    SDL_DestroyWindow(window_);
  TTF_Quit();
  SDL_Quit();
}
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
  tracks_ = {
      {"Desert Speedway", "assets/racetrack.png", "assets/mask.png", 393.0f,
       364.0f},
  };
}

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
void Menu::handleEvents() {
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
        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(tracks_.size());
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
  if (!font) return;
  SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), 0, color);
  if (!surf) return;
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
    Uint8 v = static_cast<Uint8>(10 + (i * 18) / static_cast<int>(WINDOW_HEIGHT));
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
  const float CARD_W = 420.0f;
  const float CARD_H = 300.0f; // thumbnail (236px) + name bar (64px)
  const float THUMB_H = 236.0f;
  const float GAP = 30.0f;
  const int n = static_cast<int>(tracks_.size());
  float totalW = n * CARD_W + (n - 1) * GAP;
  float startX = (WINDOW_WIDTH - totalW) / 2.0f;
  float cardY = 155.0f;

  for (int i = 0; i < n; ++i) {
    float cx = startX + i * (CARD_W + GAP);
    bool selected = (i == selectedIndex_);

    // Outer glow / border
    if (selected) {
      SDL_SetRenderDrawColor(renderer_, 255, 210, 50, 255); // gold highlight
      SDL_FRect border = {cx - 3, cardY - 3, CARD_W + 6, CARD_H + 6};
      SDL_RenderFillRect(renderer_, &border);
    } else {
      SDL_SetRenderDrawColor(renderer_, 60, 60, 80, 255);
      SDL_FRect border = {cx - 1, cardY - 1, CARD_W + 2, CARD_H + 2};
      SDL_RenderFillRect(renderer_, &border);
    }

    // Card background
    SDL_SetRenderDrawColor(renderer_, 20, 20, 38, 255);
    SDL_FRect card = {cx, cardY, CARD_W, CARD_H};
    SDL_RenderFillRect(renderer_, &card);

    // Thumbnail
    SDL_FRect thumbDst = {cx, cardY, CARD_W, THUMB_H};
    if (thumbnails_[i]) {
      SDL_RenderTexture(renderer_, thumbnails_[i], nullptr, &thumbDst);
    } else {
      // Fallback: solid colour placeholder
      SDL_SetRenderDrawColor(renderer_, 40, 80, 60, 255);
      SDL_RenderFillRect(renderer_, &thumbDst);
      renderText(renderer_, bodyFont_, "[ No Preview ]", grey,
                 cx + 140.0f, cardY + THUMB_H / 2.0f - 11.0f);
    }

    // Name bar background
    SDL_FRect nameBar = {cx, cardY + THUMB_H, CARD_W, CARD_H - THUMB_H};
    SDL_SetRenderDrawColor(renderer_, selected ? 40 : 25, selected ? 35 : 22,
                           selected ? 60 : 42, 255);
    SDL_RenderFillRect(renderer_, &nameBar);

    // Track name
    SDL_Color nameColor = selected ? gold : white;
    if (bodyFont_) {
      SDL_Surface *ns =
          TTF_RenderText_Blended(bodyFont_, tracks_[i].name.c_str(), 0, nameColor);
      if (ns) {
        SDL_Texture *nt = SDL_CreateTextureFromSurface(renderer_, ns);
        if (nt) {
          float tw = 0, th = 0;
          SDL_GetTextureSize(nt, &tw, &th);
          SDL_FRect dst = {cx + (CARD_W - tw) / 2.0f,
                           cardY + THUMB_H + (CARD_H - THUMB_H - th) / 2.0f,
                           tw, th};
          SDL_RenderTexture(renderer_, nt, nullptr, &dst);
          SDL_DestroyTexture(nt);
        }
        SDL_DestroySurface(ns);
      }
    }
  }

  // --- Bottom instructions ---
  renderText(renderer_, bodyFont_,
             "  ←  →  to select     ENTER to race     ESC to quit",
             grey, 0, WINDOW_HEIGHT - 40.0f);
  // Centre the hint
  if (bodyFont_) {
    const std::string hint =
        "  \u2190  \u2192  to select     ENTER to race     ESC to quit";
    SDL_Surface *hs = TTF_RenderText_Blended(bodyFont_, hint.c_str(), 0, grey);
    if (hs) {
      SDL_Texture *ht = SDL_CreateTextureFromSurface(renderer_, hs);
      if (ht) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(ht, &tw, &th);
        SDL_FRect dst = {(WINDOW_WIDTH - tw) / 2.0f, WINDOW_HEIGHT - 46.0f,
                         tw, th};
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
    if (t) SDL_DestroyTexture(t);
  }
  thumbnails_.clear();

  if (titleFont_) TTF_CloseFont(titleFont_);
  if (bodyFont_) TTF_CloseFont(bodyFont_);
  if (renderer_) SDL_DestroyRenderer(renderer_);
  if (window_) SDL_DestroyWindow(window_);
  TTF_Quit();
  SDL_Quit();
}
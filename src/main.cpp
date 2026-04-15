#include "engine.hpp"
#include "menu.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--dry-run") {
      std::cout << "[CI/CD] Dry run mode detected." << std::endl;
      std::cout << "Engine dependencies loaded successfully. Exiting."
                << std::endl;
      return 0; // Exit safely without initializing graphical components
    }
  }

  // Set working directory to the binary location (handles macOS .app bundles)
  const char *base_path = SDL_GetBasePath();
  if (base_path) {
    std::filesystem::current_path(base_path);
  }

  while (true) {
    // --- Show track-selection menu ---
    Menu menu;
    if (!menu.run()) {
      break; // user closed menu without selecting or hit ESC in menu
    }
    TrackInfo selected = menu.selectedTrack();

    // --- Run game with selected track ---
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    Game game("My Racing Game", 1280, 720, selected);

    while (game.running()) {
      Uint64 frameStart = SDL_GetTicks();

      game.handleEvents();
      game.update();
      game.render();

      int frameTime = static_cast<int>(SDL_GetTicks() - frameStart);
      if (frameDelay > frameTime) {
        SDL_Delay(frameDelay - frameTime);
      }
    }

    game.clean();
  }
  return 0;
}
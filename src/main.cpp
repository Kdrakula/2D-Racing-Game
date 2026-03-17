#include "engine.hpp"
#include <filesystem>
#include <SDL3/SDL.h>

int main(int argc, char* argv[]) {
    // Setup working directory for macOS app bundles (.app/Contents/Resources/)
    const char *base_path = SDL_GetBasePath();
    if (base_path) {
        std::filesystem::current_path(base_path);
    }

    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint64 frameStart;
    int frameTime;

    Game game("My Racing Game", 1280, 720);

    while (game.running()) {
        frameStart = SDL_GetTicks();

        game.handleEvents();
        game.update();
        game.render();

        frameTime = static_cast<int>(SDL_GetTicks() - frameStart);
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game.clean();
    return 0;
}
#include "SDL.H"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;

  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("SDL2 Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  int running = 1;
  while(running) {
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN) running = 0;
    }

    SDL_SetRenderDrawColor(renderer,rand() & 255,rand() & 255,rand() & 255,255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Delay(1000/60);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

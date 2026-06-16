#pragma once

#include "myanes.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

bool mn_init();
void mn_quit();

static struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  u8 joy1;
  u8 joy2;
  Uint64 last_time;
  Uint64 curr_time;
  Uint64 max_frame_time;
  Uint64 frame_time;
} imp;

SDL_AppResult SDL_AppInit(void**, int, char*[])
{
  memset(&imp, 0, sizeof(imp));

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("MyaNES", 1024, 960, 0, &imp.window, &imp.renderer)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderLogicalPresentation(imp.renderer, 256, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderVSync(imp.renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

  imp.texture = SDL_CreateTexture(imp.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!imp.texture) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(imp.texture, SDL_SCALEMODE_NEAREST);

  imp.max_frame_time = SDL_GetPerformanceFrequency();
  imp.frame_time = imp.max_frame_time / 60ULL;

  mn_input(&imp.joy1, &imp.joy2);

  if (!mn_init()) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
        case SDLK_Q: return SDL_APP_SUCCESS;
        case SDLK_R: mn_reset(); break;
        case SDLK_P: mn_power(); break;

        case SDLK_W: imp.joy1 = (imp.joy1 & ~MN_INPUT_DOWN) | MN_INPUT_UP; break;
        case SDLK_A: imp.joy1 = (imp.joy1 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT; break;
        case SDLK_S: imp.joy1 = (imp.joy1 & ~MN_INPUT_UP) | MN_INPUT_DOWN; break;
        case SDLK_D: imp.joy1 = (imp.joy1 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT; break;
        case SDLK_J: imp.joy1 |= MN_INPUT_B; break;
        case SDLK_K: imp.joy1 |= MN_INPUT_A; break;
        case SDLK_SPACE:
        case SDLK_F: imp.joy1 |= MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: imp.joy1 |= MN_INPUT_START; break;

        case SDLK_UP: imp.joy2 = (imp.joy2 & ~MN_INPUT_DOWN) | MN_INPUT_UP; break;
        case SDLK_LEFT: imp.joy2 = (imp.joy2 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT; break;
        case SDLK_DOWN: imp.joy2 = (imp.joy2 & ~MN_INPUT_UP) | MN_INPUT_DOWN; break;
        case SDLK_RIGHT: imp.joy2 = (imp.joy2 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT; break;
        case SDLK_KP_1: imp.joy2 |= MN_INPUT_B; break;
        case SDLK_KP_2: imp.joy2 |= MN_INPUT_A; break;
      }
      break;
    case SDL_EVENT_KEY_UP:
      switch (event->key.key) {
        case SDLK_W: imp.joy1 &= ~MN_INPUT_UP; break;
        case SDLK_A: imp.joy1 &= ~MN_INPUT_LEFT; break;
        case SDLK_S: imp.joy1 &= ~MN_INPUT_DOWN; break;
        case SDLK_D: imp.joy1 &= ~MN_INPUT_RIGHT; break;
        case SDLK_J: imp.joy1 &= ~MN_INPUT_B; break;
        case SDLK_K: imp.joy1 &= ~MN_INPUT_A; break;
        case SDLK_SPACE:
        case SDLK_F: imp.joy1 &= ~MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: imp.joy1 &= ~MN_INPUT_START; break;

        case SDLK_UP: imp.joy2 &= ~MN_INPUT_UP; break;
        case SDLK_LEFT: imp.joy2 &= ~MN_INPUT_LEFT; break;
        case SDLK_DOWN: imp.joy2 &= ~MN_INPUT_DOWN; break;
        case SDLK_RIGHT: imp.joy2 &= ~MN_INPUT_RIGHT; break;
        case SDLK_KP_1: imp.joy2 &= ~MN_INPUT_B; break;
        case SDLK_KP_2: imp.joy2 &= ~MN_INPUT_A; break;
      }
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
  Uint64 time = SDL_GetPerformanceCounter();
  if (imp.last_time == 0) {
    imp.last_time = time;
  }
  Uint64 diff = time - imp.last_time;
  if (diff > imp.max_frame_time) {
    diff = imp.max_frame_time;
  }
  imp.curr_time += diff;
  imp.last_time = time;

  if (imp.curr_time >= imp.frame_time) {
    void* pixels = 0;
    int pitch = 0;
    SDL_LockTexture(imp.texture, 0, &pixels, &pitch);
    mn_output(pixels);

    while (imp.curr_time >= imp.frame_time) {
      mn_frame();
      imp.curr_time -= imp.frame_time;
    }

    SDL_UnlockTexture(imp.texture);
  }

  SDL_RenderTexture(imp.renderer, imp.texture, 0, 0);
  SDL_RenderPresent(imp.renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult)
{
  mn_quit();

  SDL_DestroyTexture(imp.texture);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

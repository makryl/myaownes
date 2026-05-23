#pragma once

#include "myanes.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

bool mn_init();
void mn_quit();

static SDL_Window* window = 0;
static SDL_Renderer* renderer = 0;
static SDL_Texture* texture = 0;
static u8 joy1 = 0;
static u8 joy2 = 0;

SDL_AppResult SDL_AppInit(void**, int, char*[])
{
  SDL_SetAppMetadata("MyaNES", "1.0", "com.makryl.myanes");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("MyaNES", 1024, 960, 0, &window, &renderer)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderLogicalPresentation(renderer, 256, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!texture) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  mn_input(&joy1, &joy2);

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

        case SDLK_W: joy1 = (joy1 & ~MN_INPUT_DOWN) | MN_INPUT_UP; break;
        case SDLK_A: joy1 = (joy1 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT; break;
        case SDLK_S: joy1 = (joy1 & ~MN_INPUT_UP) | MN_INPUT_DOWN; break;
        case SDLK_D: joy1 = (joy1 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT; break;
        case SDLK_J: joy1 |= MN_INPUT_A; break;
        case SDLK_K: joy1 |= MN_INPUT_B; break;
        case SDLK_SPACE:
        case SDLK_F: joy1 |= MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: joy1 |= MN_INPUT_START; break;

        case SDLK_UP: joy2 = (joy2 & ~MN_INPUT_DOWN) | MN_INPUT_UP; break;
        case SDLK_LEFT: joy2 = (joy2 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT; break;
        case SDLK_DOWN: joy2 = (joy2 & ~MN_INPUT_UP) | MN_INPUT_DOWN; break;
        case SDLK_RIGHT: joy2 = (joy2 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT; break;
        case SDLK_KP_1: joy2 |= MN_INPUT_A; break;
        case SDLK_KP_2: joy2 |= MN_INPUT_B; break;
      }
      break;
    case SDL_EVENT_KEY_UP:
      switch (event->key.key) {
        case SDLK_W: joy1 &= ~MN_INPUT_UP; break;
        case SDLK_A: joy1 &= ~MN_INPUT_LEFT; break;
        case SDLK_S: joy1 &= ~MN_INPUT_DOWN; break;
        case SDLK_D: joy1 &= ~MN_INPUT_RIGHT; break;
        case SDLK_J: joy1 &= ~MN_INPUT_A; break;
        case SDLK_K: joy1 &= ~MN_INPUT_B; break;
        case SDLK_SPACE:
        case SDLK_F: joy1 &= ~MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: joy1 &= ~MN_INPUT_START; break;

        case SDLK_UP: joy2 &= ~MN_INPUT_UP; break;
        case SDLK_LEFT: joy2 &= ~MN_INPUT_LEFT; break;
        case SDLK_DOWN: joy2 &= ~MN_INPUT_DOWN; break;
        case SDLK_RIGHT: joy2 &= ~MN_INPUT_RIGHT; break;
        case SDLK_KP_1: joy2 &= ~MN_INPUT_A; break;
        case SDLK_KP_2: joy2 &= ~MN_INPUT_B; break;
      }
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
  void* pixels = 0;
  int pitch = 0;
  SDL_LockTexture(texture, 0, &pixels, &pitch);

  mn_output(pixels);
  mn_frame();

  SDL_UnlockTexture(texture);
  SDL_RenderTexture(renderer, texture, 0, 0);
  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult)
{
  mn_quit();

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

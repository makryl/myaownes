#pragma once

#include "myanes.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

void mn_init();
void mn_quit();

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;

SDL_AppResult SDL_AppInit(void**, int, char*[])
{
  SDL_SetAppMetadata("MyaNES", "1.0", "com.makryl.myanes");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("MyaNES", 1024, 960, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderLogicalPresentation(renderer, 256, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!texture) {
    SDL_Log("Couldn't create texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  mn_init();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
        case SDLK_Q: return SDL_APP_SUCCESS;
      }
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
  // SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  // SDL_RenderClear(renderer);

  void* screen = mn_frame();
  if (!screen) {
    return SDL_APP_SUCCESS;
  }

  SDL_UpdateTexture(texture, 0, screen, 256 * sizeof(u32));

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

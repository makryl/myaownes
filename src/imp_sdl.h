#pragma once

#include "myanes.h"
#include "common.h"

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
  bool pause;
  bool speedup;
} imp;

static const u8 imp_speedup = 4; // todo: config?

SDL_AppResult SDL_AppInit(void**, int, char*[])
{
  memset(&imp, 0, sizeof(imp));

  SDL_SetAppMetadata(MN_TITLE, MN_VERSION, MN_ID);
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(MN_TITLE, 1024, 960, 0, &imp.window, &imp.renderer)) {
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

  bool inited = mn_init();

  Rom rom = mn_rom_get();
  if (rom) {
    const char* name = rom->path;
    const char* slash = strrchr(rom->path, '/');
    const char* backslash = strrchr(rom->path, '\\');
    if (slash || backslash) {
      name = (slash > backslash ? slash : backslash) + 1;
    }

    char title[128] = {};
    strcat(title, name);
    strcat(title, " - ");
    strcat(title, MN_TITLE);

    SDL_SetWindowTitle(imp.window, title);

    SDL_Log("ROM: %s", rom->path);
    SDL_Log("Region: %s", rom->ntsc ? "NTSC" : rom->pal ? "PAL" : rom->dendy ? "Dendy" : "NOT SUPPORTED!");
    SDL_Log("Mapper: %d-%d %s", rom->mapper, rom->submapper, rom->mapper_error ? "NOT SUPPORTED!" : "");
    if (rom->prg_rom_size) {
      SDL_Log("PRG-ROM: %d KiB", rom->prg_rom_size >> 10);
    }
    if (rom->prg_ram_size) {
      SDL_Log("PRG-RAM: %d KiB %s", rom->prg_ram_size >> 10, rom->has_prg_battery ? "BATTERY" : "");
    }
    if (rom->chr_rom_size) {
      SDL_Log("CHR-ROM: %d KiB", rom->chr_rom_size >> 10);
    }
    if (rom->chr_ram_size) {
      SDL_Log("CHR-RAM: %d KiB %s", rom->chr_ram_size >> 10, rom->has_chr_battery ? "BATTERY" : "");
    }
    SDL_Log("Mirror: %s %s", rom->vert_mirror ? "vert" : "horiz", rom->alt_mirror ? ", alt (four-screen)" : "");
    SDL_Log("Trainer: %s", rom->has_trainer ? "yes" : "no");
  }

  return inited ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
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

        case SDLK_ESCAPE: imp.pause = !imp.pause; break;
        case SDLK_TAB: imp.speedup = true; break;
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

        case SDLK_TAB: imp.speedup = false; break;
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
  if (imp.pause) {
    diff = 0;
  }
  if (imp.speedup) {
    diff *= imp_speedup;
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

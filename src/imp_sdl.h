#pragma once

#include "myanes.h"
#include "common.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

bool imp_init();
void imp_quit();

static struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* tex_out;
  SDL_Texture* tex_font;
  SDL_FRect rect;
  const char* dir;
  const char* popup_text;
  Rom rom;
  char rom_info[3][33];
  u8 joy1_mask;
  u8 joy2_mask;
  u8 joy1;
  u8 joy2;
  u8 scale;
  int width;
  int height;
  Uint64 last_time;
  Uint64 curr_time;
  Uint64 max_frame_time;
  Uint64 frame_time;
  Uint64 popup_time;
  bool pause;
  bool help;
  bool fast_forward;
  bool overscan;
} imp;

static const u8 imp_speedup = 4; // todo: config?

static void imp_draw_help()
{
  SDL_FRect fill = { 0, 0, 256, 240 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xC0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);

  u8 i = 0;
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0x80, 0xFF, 0xFF);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Help          F1    MyaowNES  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Open          F2      v" MN_VERSION "  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Reload        F3              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Reset         F4              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Quick save    F5   /\\____/\\   ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Quick load    F8              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Quit         F10  |  o..o  |  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Fullscreen   F11  |-[ + oo]|  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Screenshot   F12  |        |  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Save slot    1-9              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Fast forward Tab  [_m____m_]  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Pause        Esc              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Scale        -/+              ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "              Joy1        Joy2  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  D-pad       WASD      Arrows  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  B              J       NUM_1  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  A              K       NUM_2  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  B turbo        U       NUM_4  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  A turbo        I       NUM_5  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Start          H       Enter  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "  Select         F       Space  ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, imp.rom_info[0]);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, imp.rom_info[1]);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, imp.rom_info[2]);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
}

static void imp_draw_popup(Uint64 time_diff)
{
  imp.popup_time -= imp.popup_time > time_diff ? time_diff : imp.popup_time;
  SDL_FRect fill = { 16, 16, strlen(imp.popup_text) * 8 + 3, 11 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xC0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0x88, 0xFF, 0xFF);
  SDL_RenderDebugText(imp.renderer, 18, 18, imp.popup_text);
}

static void imp_popup(const char* text, float time)
{
  imp.popup_text = text;
  imp.popup_time = imp.max_frame_time * time;
}

static void imp_update_frame_time()
{
  imp.max_frame_time = SDL_GetPerformanceFrequency();
  imp.frame_time = imp.max_frame_time / 60ULL; // todo: PAL
}

static void imp_update_size() { SDL_SetWindowSize(imp.window, 320 * imp.scale, 240 * imp.scale); }

static void imp_update_output_rect()
{
  mn_output_rect(imp.overscan, imp.width, imp.height, &imp.rect.x, &imp.rect.y, &imp.rect.w, &imp.rect.h);
}

static void imp_rom_update()
{
  Rom rom = mn_rom_get();
  if (!rom) {
    imp.help = true;
    imp.pause = true;
    return;
  }

  const char* name = rom->path;
  const char* slash = strrchr(rom->path, '/');
  const char* backslash = strrchr(rom->path, '\\');
  if (slash || backslash) {
    name = (slash > backslash ? slash : backslash) + 1;
  }

  char title[128] = {};
  snprintf(title, sizeof(title), "%s - MyaowNES", name);

  SDL_SetWindowTitle(imp.window, title);

  memset(imp.rom_info, 0, sizeof(imp.rom_info));
  snprintf(imp.rom_info[0], sizeof(imp.rom_info[0]), "  %-5s %03d-%03d%s  %-5s %-3s %3s  ",
           rom->ntsc    ? "NTSC"
           : rom->pal   ? "PAL"
           : rom->dendy ? "Dendy"
                        : "Error",
           rom->mapper, rom->submapper, rom->mapper_error ? "!" : "", rom->vert_mirror ? "Vert" : "Horiz",
           rom->alt_mirror ? "Alt" : "", rom->has_trainer ? "512" : "");
  snprintf(imp.rom_info[1], sizeof(imp.rom_info[1]), "  PRG-ROM %4dK %8s %4dK  ", rom->prg_rom_size >> 10,
           rom->has_prg_battery ? "PRG-SRAM" : "PRG-RAM", rom->prg_ram_size >> 10);
  snprintf(imp.rom_info[2], sizeof(imp.rom_info[2]), "  CHR-ROM %4dK %8s %4dK  ", rom->chr_rom_size >> 10,
           rom->has_chr_battery ? "CHR-SRAM" : "CHR-RAM", rom->chr_ram_size >> 10);

  imp_update_frame_time();
}

SDL_AppResult SDL_AppInit(void**, int argc, char* argv[])
{
  memset(&imp, 0, sizeof(imp));
  imp.scale = 4;
  imp.overscan = true;

  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      SDL_Log("MyaowNES v" MN_VERSION "\n");
      SDL_Log("Usage: myaownes [options] [file]\n");
      SDL_Log("Options:\n");
      SDL_Log("-h, --help             Display help\n");
      SDL_Log("-d, --dir <path>       Save directory. Default: rom file dir\n");
      SDL_Log("-s, --scale <n>        Scale. Default: 4\n");
      SDL_Log("-o, --overscan <1|0>   Overscan. Default: 1\n");
      return SDL_APP_SUCCESS;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
      imp.dir = argv[i++];
    } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--scale") == 0) {
      imp.scale = SDL_strtol(argv[i++], nullptr, 10);
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--overscan") == 0) {
      imp.overscan = SDL_strtol(argv[i++], nullptr, 10);
    } else {
    }
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("MyaowNES", 320 * imp.scale, 240 * imp.scale, SDL_WINDOW_RESIZABLE, &imp.window,
                                   &imp.renderer))
  {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderVSync(imp.renderer, 1);

  imp.tex_out = SDL_CreateTexture(imp.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET, 256, 240);
  if (!imp.tex_out) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(imp.tex_out, SDL_SCALEMODE_PIXELART);

  imp_update_frame_time();

  bool inited = imp_init();
  imp_rom_update();

  return inited ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

static void SDLCALL imp_file_dialog_cb(void*, const char* const* files, int)
{
  if (!files || !*files) {
    return;
  }
  if (mn_rom_get()) {
    mn_rom_release(mn_rom_get());
  }
  imp.rom = mn_rom_file(files[0]);
  mn_rom_set(imp.rom);
  imp_rom_update();
  imp.help = false;
  imp.pause = false;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      SDL_GetWindowSizeInPixels(imp.window, &imp.width, &imp.height);
      imp_update_output_rect();
      break;
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
        case SDLK_F10: return SDL_APP_SUCCESS;
        case SDLK_F1:
          if (imp.help) {
            imp.help = false;
            imp.pause = false;
          } else {
            imp.help = true;
            imp.pause = true;
            imp_draw_help();
          }
          break;
        case SDLK_F2:
          SDL_DialogFileFilter filters[] = { { "iNES / NES 2.0 ROMs (*.nes)", "nes" }, { "All Files (*.*)", "*" } };
          SDL_ShowOpenFileDialog(imp_file_dialog_cb, nullptr, imp.window, filters, 2, nullptr, false);
          break;
        case SDLK_F3: mn_rom_set(mn_rom_get()); break;
        case SDLK_F4: mn_reset(); break;
        case SDLK_F11:
          Uint32 flags = SDL_GetWindowFlags(imp.window);
          bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
          SDL_SetWindowFullscreen(imp.window, !is_fullscreen);
          break;
        case SDLK_EQUALS:
          ++imp.scale;
          imp_update_size();
          break;
        case SDLK_MINUS:
          if (imp.scale > 1) {
            --imp.scale;
          }
          imp_update_size();
          break;
        case SDLK_0:
          imp.overscan = !imp.overscan;
          imp_update_output_rect();
          break;
        case SDLK_ESCAPE:
          if (imp.help) {
            imp.help = false;
            imp.pause = false;
          } else {
            imp.pause = !imp.pause;
            if (imp.pause) {
              imp_popup("Pause", 1);
            }
          }
          break;
        case SDLK_TAB:
          imp.fast_forward = true;
          imp_popup(">>", 0.5);
          break;

        case SDLK_W:
          imp.joy1_mask |= MN_INPUT_UP;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_DOWN) | MN_INPUT_UP;
          break;
        case SDLK_A:
          imp.joy1_mask |= MN_INPUT_LEFT;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT;
          break;
        case SDLK_S:
          imp.joy1_mask |= MN_INPUT_DOWN;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_UP) | MN_INPUT_DOWN;
          break;
        case SDLK_D:
          imp.joy1_mask |= MN_INPUT_RIGHT;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT;
          break;
        case SDLK_J: imp.joy1 |= MN_INPUT_B; break;
        case SDLK_K: imp.joy1 |= MN_INPUT_A; break;
        case SDLK_SPACE:
        case SDLK_F: imp.joy1 |= MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: imp.joy1 |= MN_INPUT_START; break;

        case SDLK_UP:
          imp.joy2_mask |= MN_INPUT_UP;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_DOWN) | MN_INPUT_UP;
          break;
        case SDLK_LEFT:
          imp.joy2_mask |= MN_INPUT_LEFT;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT;
          break;
        case SDLK_DOWN:
          imp.joy2_mask |= MN_INPUT_DOWN;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_UP) | MN_INPUT_DOWN;
          break;
        case SDLK_RIGHT:
          imp.joy2_mask |= MN_INPUT_RIGHT;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT;
          break;
        case SDLK_KP_1: imp.joy2 |= MN_INPUT_B; break;
        case SDLK_KP_2: imp.joy2 |= MN_INPUT_A; break;
      }
      break;
    case SDL_EVENT_KEY_UP:
      switch (event->key.key) {
        case SDLK_TAB: imp.fast_forward = false; break;

        case SDLK_W:
          imp.joy1_mask &= ~MN_INPUT_UP;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_UP) | (imp.joy1_mask & MN_INPUT_DOWN);
          break;
        case SDLK_A:
          imp.joy1_mask &= ~MN_INPUT_LEFT;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_LEFT) | (imp.joy1_mask & MN_INPUT_RIGHT);
          break;
        case SDLK_S:
          imp.joy1_mask &= ~MN_INPUT_DOWN;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_DOWN) | (imp.joy1_mask & MN_INPUT_UP);
          break;
        case SDLK_D:
          imp.joy1_mask &= ~MN_INPUT_RIGHT;
          imp.joy1 = (imp.joy1 & ~MN_INPUT_RIGHT) | (imp.joy1_mask & MN_INPUT_LEFT);
          break;
        case SDLK_J: imp.joy1 &= ~MN_INPUT_B; break;
        case SDLK_K: imp.joy1 &= ~MN_INPUT_A; break;
        case SDLK_SPACE:
        case SDLK_F: imp.joy1 &= ~MN_INPUT_SELECT; break;
        case SDLK_RETURN:
        case SDLK_H: imp.joy1 &= ~MN_INPUT_START; break;

        case SDLK_UP:
          imp.joy2_mask &= ~MN_INPUT_UP;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_UP) | (imp.joy2_mask & MN_INPUT_DOWN);
          break;
        case SDLK_LEFT:
          imp.joy2_mask &= ~MN_INPUT_LEFT;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_LEFT) | (imp.joy2_mask & MN_INPUT_RIGHT);
          break;
        case SDLK_DOWN:
          imp.joy2_mask &= ~MN_INPUT_DOWN;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_DOWN) | (imp.joy2_mask & MN_INPUT_UP);
          break;
        case SDLK_RIGHT:
          imp.joy2_mask &= ~MN_INPUT_RIGHT;
          imp.joy2 = (imp.joy2 & ~MN_INPUT_RIGHT) | (imp.joy2_mask & MN_INPUT_LEFT);
          break;
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
  Uint64 time_diff = time - imp.last_time;
  Uint64 game_diff = time_diff;
  if (game_diff > imp.max_frame_time) {
    game_diff = imp.max_frame_time;
  }
  if (imp.pause) {
    game_diff = 0;
  } else if (imp.fast_forward) {
    game_diff *= imp_speedup;
  }
  imp.curr_time += game_diff;
  imp.last_time = time;

  if (imp.curr_time >= imp.frame_time) {
    while (imp.curr_time >= imp.frame_time) {
      mn_frame(imp.joy1, imp.joy2);
      imp.curr_time -= imp.frame_time;
    }
  }

  SDL_UpdateTexture(imp.tex_out, nullptr, mn_output(), 256 * 4);
  SDL_SetRenderTarget(imp.renderer, imp.tex_out);
  if (imp.help) {
    imp_draw_help();
  } else if (imp.popup_time > 0) {
    imp_draw_popup(time_diff);
  }

  SDL_SetRenderTarget(imp.renderer, nullptr);
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xFF);
  SDL_RenderClear(imp.renderer);

  SDL_RenderTexture(imp.renderer, imp.tex_out, nullptr, &imp.rect);
  SDL_RenderPresent(imp.renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult)
{
  imp_quit();

  if (mn_rom_get()) {
    mn_rom_release(mn_rom_get());
  }

  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

#pragma once

#include "myaownes.h"
#include "common.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

bool imp_init();
void imp_quit();

enum
{
  IMP_PATH_SIZE = 4096,
};

static struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* tex_out;
  SDL_Texture* tex_font;
  SDL_FRect rect;
  const char* popup_text;
  Uint64 last_time;
  Uint64 curr_time;
  Uint64 max_frame_time;
  Uint64 target_frame_time;
  Uint64 real_frame_time;
  Uint64 popup_time;
  Uint64 fps_time;
  Uint64 fps_count;
  Uint64 fps_value;
  Uint64 auto_save_time;
  Uint64 perf_freq;
  uint auto_save_period;
  uint scale;
  uint turbo;
  uint slot;
  u8 joy1_mask;
  u8 joy2_mask;
  u8 joy1;
  u8 joy2;
  bool pause;
  bool help;
  bool fps;
  bool fast_forward;
  bool auto_aspect;
  bool overscan;
  bool joy1_turbo_a;
  bool joy1_turbo_b;
  bool joy2_turbo_a;
  bool joy2_turbo_b;
  bool dirty_config;
  Rom rom;
  char rom_info[6][15];
  char rom_path[IMP_PATH_SIZE];
  char sram_path[IMP_PATH_SIZE];
  char save_dir[IMP_PATH_SIZE];
  char config_path[IMP_PATH_SIZE];
} imp;

static void imp_draw_help()
{
  SDL_FRect fill = { 0, 0, 256, 240 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xC0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);

  u8 i = 2;
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0x80, 0xFF, 0xFF);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Help        F1        MyaowNES ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Open        F2          v" MN_VERSION " ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Reload      F3                 ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Reset       F4    /\\____/\\     ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Quick save  F5                 ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Aspect      F6   |  o..o  |    ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Overscan    F7   |-[ + oo]|    ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Quick load  F8   |        |    ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Quit       F10   [_m____m_]    ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Fullscreen F11                 ");
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " Screenshot F12  %s ", imp.rom_info[0]);
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " Save slot  0-9  %s ", imp.rom_info[1]);
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " F-forward  Tab  %s ", imp.rom_info[2]);
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " Pause      Esc  %s ", imp.rom_info[3]);
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " Scale      -/+  %s ", imp.rom_info[4]);
  SDL_RenderDebugTextFormat(imp.renderer, 0, 8 * i++, " FPS          `  %s ", imp.rom_info[5]);
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, "           Joy1    Joy2          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " D-pad     WASD  Arrows          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " B            J   NUM_1          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " A            K   NUM_2          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " B turbo      U   NUM_4          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " A turbo      I   NUM_5          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Start        H   Enter          ");
  SDL_RenderDebugText(imp.renderer, 0, 8 * i++, " Select       F   Space          ");
}

static void imp_draw_popup()
{
  imp.popup_time -= imp.popup_time > imp.real_frame_time ? imp.real_frame_time : imp.popup_time;
  SDL_FRect fill = { 16, 16, SDL_strlen(imp.popup_text) * 8 + 3, 11 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xC0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0x88, 0xFF, 0xFF);
  SDL_RenderDebugText(imp.renderer, 18, 18, imp.popup_text);
}

static void imp_popup(const char* text, float time)
{
  imp.popup_text = text;
  imp.popup_time = imp.perf_freq * time;
}

static void imp_update_frame_time()
{
  imp.max_frame_time = imp.perf_freq / 5;
  imp.target_frame_time = imp.perf_freq / 60; // todo: PAL
}

static void imp_update_size()
{
  uint width;
  uint height;
  mn_output_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);
  SDL_SetWindowSize(imp.window, width, height);
}

static const char* imp_rom_name()
{
  const char* name = imp.rom_path;
  const char* slash = SDL_strrchr(imp.rom_path, '/');
  const char* backslash = SDL_strrchr(imp.rom_path, '\\');
  if (slash || backslash) {
    name = (slash > backslash ? slash : backslash) + 1;
  }
  return name;
}

static void imp_save_path(char* dst, const char* ext)
{
  dst[0] = 0;
  SDL_strlcat(dst, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(dst, imp_rom_name(), IMP_PATH_SIZE);
  char* dot = SDL_strrchr(dst, '.');
  if (dot) {
    *dot = 0;
  }
  SDL_strlcat(dst, ext, IMP_PATH_SIZE);
}

static void imp_rom_update()
{
  Rom rom = imp.rom;
  if (!rom) {
    imp.help = true;
    imp.pause = true;
    return;
  }
  imp.help = false;
  imp.pause = false;

  char title[256] = {};
  SDL_strlcat(title, imp_rom_name(), IMP_PATH_SIZE);
  SDL_strlcat(title, " - MyaowNES", IMP_PATH_SIZE);

  SDL_SetWindowTitle(imp.window, title);

  SDL_memset(imp.rom_info, 0, sizeof(imp.rom_info));
  SDL_snprintf(imp.rom_info[0], sizeof(imp.rom_info[0]), "%-5s %1s%03d-%03d",
               rom->ntsc    ? "NTSC"
               : rom->pal   ? "PAL"
               : rom->dendy ? "Dendy"
                            : "Error",
               rom->mapper_error ? "!" : " ", rom->mapper, rom->submapper);
  SDL_snprintf(imp.rom_info[1], sizeof(imp.rom_info[0]), "%-5s %-3s %4s", rom->vert_mirror ? "Vert" : "Horiz",
               rom->alt_mirror ? "Alt" : "", rom->has_trainer ? "TR" : "");
  SDL_snprintf(imp.rom_info[2], sizeof(imp.rom_info[0]), "PRG-ROM %5dK", rom->prg_rom_size >> 10);
  SDL_snprintf(imp.rom_info[3], sizeof(imp.rom_info[0]), "PRG-%-4s %4dK", rom->prg_has_battery ? "SRAM" : "RAM",
               rom->prg_ram_size >> 10);
  SDL_snprintf(imp.rom_info[4], sizeof(imp.rom_info[0]), "CHR-ROM %5dK", rom->chr_rom_size >> 10);
  SDL_snprintf(imp.rom_info[5], sizeof(imp.rom_info[0]), "CHR-%-4s %4dK", rom->chr_has_battery ? "SRAM" : "RAM",
               rom->chr_ram_size >> 10);

  for (uint i = 0; i < 6; ++i) {
    SDL_Log("%s", imp.rom_info[i]);
  }

  imp_update_frame_time();
}

static void imp_save_config()
{
  if (!imp.dirty_config) {
    return;
  }
  FILE* f = fopen(imp.config_path, "w");
  if (!f) {
    return;
  }
  fprintf(f, "scale %u\n", imp.scale);
  fprintf(f, "auto_aspect %u\n", imp.auto_aspect);
  fprintf(f, "overscan %u\n", imp.overscan);
  fprintf(f, "auto_save_period %u\n", imp.auto_save_period);
  fprintf(f, "fps %u\n", imp.fps);
  fclose(f);
  imp.dirty_config = false;
}

static void imp_load_config()
{
  imp.scale = 2;
  imp.auto_aspect = true;
  imp.overscan = true;
  imp.auto_save_period = 60;
  imp.fps = false;

  FILE* f = fopen(imp.config_path, "r");
  if (!f) {
    return;
  }
  char key[32];
  while (fscanf(f, "%31s", key) == 1) {
    if (strcmp(key, "scale") == 0) {
      fscanf(f, "%u", &imp.scale);
    } else if (strcmp(key, "auto_aspect") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.auto_aspect = val;
    } else if (strcmp(key, "overscan") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.overscan = val;
    } else if (strcmp(key, "auto_save_period") == 0) {
      fscanf(f, "%u", &imp.auto_save_period);
    } else if (strcmp(key, "fps") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.fps = val;
    }
  }
  fclose(f);
}

static void imp_rom_unload()
{
  if (imp.rom) {
    if ((imp.rom->prg_has_battery || imp.rom->chr_has_battery)) {
      mn_rom_save(imp.sram_path);
    }
    mn_rom_release(imp.rom);
  }
}

static void imp_rom_load(const char* path)
{
  imp_rom_unload();
  SDL_strlcpy(imp.rom_path, path, IMP_PATH_SIZE);
  imp_save_path(imp.sram_path, ".sav");
  imp.rom = mn_rom_load(imp.rom_path, imp.sram_path);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
}

static void imp_save()
{
  char qs_path[IMP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  imp_save_path(qs_path, ext);
  if (mn_save(qs_path)) {
    imp_popup("Saved", 2);
  }
}

static void imp_load()
{
  char qs_path[IMP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  imp_save_path(qs_path, ext);
  if (mn_load(qs_path)) {
    imp_popup("Loaded", 2);
  }
}

static void SDLCALL imp_file_dialog_cb(void*, const char* const* files, int)
{
  if (!files || !*files) {
    return;
  }
  imp_rom_load(files[0]);
  imp_rom_update();
}

SDL_AppResult SDL_AppInit(void**, int argc, char* argv[])
{
  SDL_Log("MyaowNES v%s", MN_VERSION);
  SDL_memset(&imp, 0, sizeof(imp));

  for (int i = 0; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "-h") == 0 || SDL_strcmp(argv[i], "--help") == 0) {
      SDL_Log("MyaowNES v" MN_VERSION);
      SDL_Log("Usage: myaownes [options] [file]");
      SDL_Log("Options:");
      SDL_Log("-h, --help         Display help");
      SDL_Log("-p, --path <path>  Save path");
      return SDL_APP_SUCCESS;
    } else if (SDL_strcmp(argv[i], "-p") == 0 || SDL_strcmp(argv[i], "--path") == 0) {
      SDL_strlcpy(imp.save_dir, argv[++i], IMP_PATH_SIZE);
      uint len = SDL_strlen(imp.save_dir);
      if (len > 0 && imp.save_dir[len - 1] != '/' && imp.save_dir[len - 1] != '\\') {
        SDL_strlcat(imp.save_dir, "/", IMP_PATH_SIZE);
      }
    } else if (i > 0) {
      imp_rom_load(argv[i]);
    }
  }

  if (SDL_strlen(imp.save_dir) == 0) {
    char* pref_path = SDL_GetPrefPath(nullptr, "MyaowNES");
    SDL_strlcpy(imp.save_dir, pref_path, IMP_PATH_SIZE);
    SDL_free(pref_path);
  }

  SDL_Log("Save path: %s", imp.save_dir);

  SDL_strlcat(imp.config_path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(imp.config_path, "config.txt", IMP_PATH_SIZE);

  imp_load_config();

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  imp.perf_freq = SDL_GetPerformanceFrequency();

  uint width;
  uint height;
  mn_output_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);
  if (!SDL_CreateWindowAndRenderer("MyaowNES", width, height, SDL_WINDOW_RESIZABLE, &imp.window, &imp.renderer)) {
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

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
      int width;
      int height;
      SDL_GetWindowSizeInPixels(imp.window, &width, &height);
      mn_output_fit(imp.auto_aspect, imp.overscan, width, height, &imp.rect.x, &imp.rect.y, &imp.rect.w, &imp.rect.h);
      break;
    }
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
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
        case SDLK_F3:
          if (imp.rom) {
            mn_rom_set(imp.rom);
          }
          break;
        case SDLK_F4: mn_reset(); break;
        case SDLK_F5: imp_save(); break;
        case SDLK_F6:
          imp.auto_aspect = !imp.auto_aspect;
          imp.dirty_config = true;
          imp_update_size();
          imp_popup(imp.auto_aspect ? "Aspect ON" : "Aspect OFF", 1);
          break;
        case SDLK_F7:
          imp.overscan = !imp.overscan;
          imp.dirty_config = true;
          imp_update_size();
          imp_popup(imp.overscan ? "Overscan ON" : "Overscan OFF", 1);
          break;
        case SDLK_F8: imp_load(); break;
        case SDLK_F10: return SDL_APP_SUCCESS;
        case SDLK_F11:
          Uint32 flags = SDL_GetWindowFlags(imp.window);
          bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
          SDL_SetWindowFullscreen(imp.window, !is_fullscreen);
          break;
        case SDLK_F12: break; // todo screenshot
        case SDLK_EQUALS:
          if (imp.scale < 8) {
            ++imp.scale;
            imp.dirty_config = true;
          }
          switch (imp.scale) {
            case 1: imp_popup("Scale x1", 1); break;
            case 2: imp_popup("Scale x2", 1); break;
            case 3: imp_popup("Scale x3", 1); break;
            case 4: imp_popup("Scale x4", 1); break;
            case 5: imp_popup("Scale x5", 1); break;
            case 6: imp_popup("Scale x6", 1); break;
            case 7: imp_popup("Scale x7", 1); break;
            case 8: imp_popup("Scale x8", 1); break;
          }
          imp_update_size();
          break;
        case SDLK_MINUS:
          if (imp.scale > 1) {
            --imp.scale;
            imp.dirty_config = true;
          }
          switch (imp.scale) {
            case 1: imp_popup("Scale x1", 1); break;
            case 2: imp_popup("Scale x2", 1); break;
            case 3: imp_popup("Scale x3", 1); break;
            case 4: imp_popup("Scale x4", 1); break;
            case 5: imp_popup("Scale x5", 1); break;
            case 6: imp_popup("Scale x6", 1); break;
            case 7: imp_popup("Scale x7", 1); break;
            case 8: imp_popup("Scale x8", 1); break;
          }
          imp_update_size();
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
          SDL_SetRenderVSync(imp.renderer, 0);
          break;
        case SDLK_GRAVE:
          imp.fps = !imp.fps;
          imp.dirty_config = true;
          break;
        case SDLK_0:
          imp.slot = 0;
          imp_popup("Slot 0", 2);
          break;
        case SDLK_1:
          imp.slot = 1;
          imp_popup("Slot 1", 2);
          break;
        case SDLK_2:
          imp.slot = 2;
          imp_popup("Slot 2", 2);
          break;
        case SDLK_3:
          imp.slot = 3;
          imp_popup("Slot 3", 2);
          break;
        case SDLK_4:
          imp.slot = 4;
          imp_popup("Slot 4", 2);
          break;
        case SDLK_5:
          imp.slot = 5;
          imp_popup("Slot 5", 2);
          break;
        case SDLK_6:
          imp.slot = 6;
          imp_popup("Slot 6", 2);
          break;
        case SDLK_7:
          imp.slot = 7;
          imp_popup("Slot 7", 2);
          break;
        case SDLK_8:
          imp.slot = 8;
          imp_popup("Slot 8", 2);
          break;
        case SDLK_9:
          imp.slot = 9;
          imp_popup("Slot 9", 2);
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
        case SDLK_U: imp.joy1_turbo_b = true; break;
        case SDLK_I: imp.joy1_turbo_a = true; break;
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
        case SDLK_KP_4: imp.joy2_turbo_b = true; break;
        case SDLK_KP_5: imp.joy2_turbo_a = true; break;
      }
      break;
    case SDL_EVENT_KEY_UP:
      switch (event->key.key) {
        case SDLK_TAB:
          imp.fast_forward = false;
          SDL_SetRenderVSync(imp.renderer, 1);
          break;

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
        case SDLK_U: imp.joy1_turbo_b = false; break;
        case SDLK_I: imp.joy1_turbo_a = false; break;
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
        case SDLK_KP_4: imp.joy2_turbo_b = false; break;
        case SDLK_KP_5: imp.joy2_turbo_a = false; break;
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
  imp.real_frame_time = time - imp.last_time;
  Uint64 game_diff = imp.real_frame_time;
  if (imp.pause) {
    game_diff = 0;
  } else if (imp.fast_forward) {
    game_diff = imp.target_frame_time;
  }
  imp.curr_time += game_diff;
  imp.last_time = time;

  if (imp.curr_time >= imp.target_frame_time) {
    if (imp.fps) {
      ++imp.fps_count;
    }
    if (++imp.turbo & 1) {
      if (imp.joy1_turbo_a) {
        imp.joy1 ^= MN_INPUT_A;
      }
      if (imp.joy1_turbo_b) {
        imp.joy1 ^= MN_INPUT_B;
      }
      if (imp.joy2_turbo_a) {
        imp.joy2 ^= MN_INPUT_A;
      }
      if (imp.joy2_turbo_b) {
        imp.joy2 ^= MN_INPUT_B;
      }
    }
    mn_frame(imp.joy1, imp.joy2);
    imp.curr_time -= imp.curr_time > imp.target_frame_time ? imp.target_frame_time : imp.curr_time;
  }

  if (imp.fps) {
    imp.fps_time += imp.real_frame_time;
    if (imp.fps_time > imp.perf_freq) {
      imp.fps_value = imp.fps_count;
      imp.fps_time -= imp.perf_freq;
      imp.fps_count = 0;
    }
  }

  imp.auto_save_time += imp.real_frame_time;
  if (imp.auto_save_time >= imp.auto_save_period * imp.perf_freq) {
    imp.auto_save_time -= imp.auto_save_period * imp.perf_freq;
    mn_rom_save(imp.sram_path);
    imp_save_config();
  }

  SDL_UpdateTexture(imp.tex_out, nullptr, mn_output(), 256 * 4);
  SDL_SetRenderTarget(imp.renderer, imp.tex_out);
  if (imp.help) {
    imp_draw_help();
  } else if (imp.popup_time > 0) {
    imp_draw_popup();
  }
  if (imp.fps) {
    SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0x88, 0xFF, 0xFF);
    SDL_RenderDebugTextFormat(imp.renderer, 256 - 18 - 4 * 8, 18, "%4d", (int)imp.fps_value);
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
  imp_rom_unload();
  imp_save_config();
  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

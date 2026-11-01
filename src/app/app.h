#pragma once

#include "myaownes.h"
#include <SDL3/SDL.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

enum
{
  IMP_PATH_SIZE = 4096,
  IMP_RECENT_COUNT = 20,
};

enum
{
  IMP_MENU_OFF = 0,
  IMP_MENU_MAIN,
  IMP_MENU_OPTIONS,
  IMP_MENU_INFO,
  IMP_MENU_HELP,
  IMP_MENU_RECENT,
  IMP_MENU_START,
};

enum
{
  IMP_TOUCH_A = 0,
  IMP_TOUCH_B,
  IMP_TOUCH_START,
  IMP_TOUCH_SELECT,
  IMP_TOUCH_FF,
  IMP_TOUCH_UP,
  IMP_TOUCH_DOWN,
  IMP_TOUCH_LEFT,
  IMP_TOUCH_RIGHT,
  IMP_TOUCH_COUNT,
};

typedef enum
{
  IMP_ACTION_NONE = 0,
  IMP_ACTION_JOY1_A,
  IMP_ACTION_JOY1_B,
  IMP_ACTION_JOY1_A_TURBO,
  IMP_ACTION_JOY1_B_TURBO,
  IMP_ACTION_JOY1_START,
  IMP_ACTION_JOY1_SELECT,
  IMP_ACTION_JOY1_UP,
  IMP_ACTION_JOY1_DOWN,
  IMP_ACTION_JOY1_LEFT,
  IMP_ACTION_JOY1_RIGHT,
  IMP_ACTION_JOY2_A,
  IMP_ACTION_JOY2_B,
  IMP_ACTION_JOY2_A_TURBO,
  IMP_ACTION_JOY2_B_TURBO,
  IMP_ACTION_JOY2_START,
  IMP_ACTION_JOY2_SELECT,
  IMP_ACTION_JOY2_UP,
  IMP_ACTION_JOY2_DOWN,
  IMP_ACTION_JOY2_LEFT,
  IMP_ACTION_JOY2_RIGHT,
  IMP_ACTION_MENU,
  IMP_ACTION_HELP,
  IMP_ACTION_OPEN,
  IMP_ACTION_REGION,
  IMP_ACTION_RESET,
  IMP_ACTION_SAVE,
  IMP_ACTION_ASPECT,
  IMP_ACTION_OVERSCAN,
  IMP_ACTION_LOAD,
  IMP_ACTION_QUIT,
  IMP_ACTION_FULLSCREEN,
  IMP_ACTION_SCREENSHOT,
  IMP_ACTION_PAUSE,
  IMP_ACTION_SHOW_FPS,
  IMP_ACTION_MAX_FPS,
  IMP_ACTION_FAST_FORWARD,
  IMP_ACTION_SLOT0,
  IMP_ACTION_SLOT1,
  IMP_ACTION_SLOT2,
  IMP_ACTION_SLOT3,
  IMP_ACTION_SLOT4,
  IMP_ACTION_SLOT5,
  IMP_ACTION_SLOT6,
  IMP_ACTION_SLOT7,
  IMP_ACTION_SLOT8,
  IMP_ACTION_SLOT9,
  IMP_ACTION_SLOT_NEXT,
  IMP_ACTION_SLOT_PREV,
  IMP_ACTION_VOLUME_UP,
  IMP_ACTION_VOLUME_DOWN,
  IMP_ACTION_SCALE_UP,
  IMP_ACTION_SCALE_DOWN,
  IMP_ACTION_COUNT,
} imp_action;

typedef struct
{
  const char* label;
  void (*onkeydown)();
  void (*onkeyup)();
} imp_action_data;

static const imp_action_data imp_actions[IMP_ACTION_COUNT];

static struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* tex_out;
  SDL_Texture* tex_font;
  SDL_Gamepad* gamepad1;
  SDL_Gamepad* gamepad2;
  SDL_AudioDeviceID audioDeviceId;
  SDL_AudioStream* stream;
  SDL_JoystickID gamepad_id1;
  SDL_JoystickID gamepad_id2;
  SDL_FRect rect;
  SDL_FRect ui_offset;
  const char* popup_text;
  u64 last_time;
  u64 curr_time;
  u64 target_frame_time;
  u64 real_frame_time;
  u64 popup_time;
  u64 fps_time;
  u64 fps_count;
  u64 fps_value;
  u64 auto_save_time;
  u64 perf_freq;
  u64 touch_button[IMP_TOUCH_COUNT];
  uint auto_save_period;
  uint volume;
  uint scale;
  uint slot;
  uint audio_buffer_size;
  uint fast_forward_scale;
  uint region;
  // uint filter_hp;
  // uint filter_lp;
  uint joy1;
  uint joy2;
  uint menu_state;
  uint menu_state_prev;
  int menu_selection;
  uint menu_input;
  uint menu_input_prev;
  int touch_button_radius;
  int touch_button_ab_radius;
  uint touch_a_x;
  uint touch_a_y;
  uint touch_b_x;
  uint touch_b_y;
  uint touch_start_x;
  uint touch_start_y;
  uint touch_select_x;
  uint touch_select_y;
  uint touch_menu_x;
  uint touch_menu_y;
  uint touch_ff_x;
  uint touch_ff_y;
  int touch_dpad_radius;
  int touch_dpad_threshold;
  uint touch_dpad_x;
  uint touch_dpad_y;
  int width;
  int height;
  SDL_AtomicInt load_rom_dialog_done;
  bool pause;
  bool fps;
  bool fps_test;
  bool fast_forward;
  bool auto_aspect;
  bool overscan;
  bool dirty_config;
  bool touch_active;
  bool load_rom_dialog_wait;
  bool embed_enabled;
  bool start_screen;
  bool quit;
  mn_rom rom;
  mn_tas tas;
  char rom_info[6][15];
  char load_rom_path[IMP_PATH_SIZE];
  char rom_path[IMP_PATH_SIZE];
  char sram_path[IMP_PATH_SIZE];
  char save_dir[IMP_PATH_SIZE];
  char default_config_path[IMP_PATH_SIZE];
  char config_path[IMP_PATH_SIZE];
  char history_path[IMP_PATH_SIZE];
  char palette_path[IMP_PATH_SIZE];
  char recent_files[IMP_RECENT_COUNT][IMP_PATH_SIZE];
  SDL_Scancode scancode_by_action[IMP_ACTION_COUNT];
  imp_action action_by_scancode[SDL_SCANCODE_COUNT];
} imp;

static const char* const imp_save_slot_labels[10] = { "Slot 0", "Slot 1", "Slot 2", "Slot 3", "Slot 4",
                                                      "Slot 5", "Slot 6", "Slot 7", "Slot 8", "Slot 9" };
static const char* const imp_scale_labels[8] = { "Scale 1", "Scale 2", "Scale 3", "Scale 4",
                                                 "Scale 5", "Scale 6", "Scale 7", "Scale 8" };
static const char* const imp_volume_labels[11] = { "Volume OFF", "Volume 1", "Volume 2", "Volume 3",
                                                   "Volume 4",   "Volume 5", "Volume 6", "Volume 7",
                                                   "Volume 8",   "Volume 9", "Volume 10" };
static const char* const imp_region_labels[4] = { "Region NTSC", "Region PAL", "Region Auto", "Region Dendy" };
static const char* const imp_aspect_labels[2] = { "Aspect OFF", "Aspect ON" };
static const char* const imp_overscan_labels[2] = { "Overscan OFF", "Overscan ON" };
static const char* const imp_fps_labels[2] = { "Show FPS OFF", "Show FPS ON" };
static const char* const imp_fps_test_labels[2] = { "Max FPS OFF", "Max FPS ON" };
static const char* const imp_fullscreen_labels[2] = { "Fullscreen OFF", "Fullscreen ON" };
static const char* const imp_fast_forward_labels[4] = { "Fast forward x2", "Fast forward x3", "Fast forward x4",
                                                        "Fast forward x5" };
static const char* const imp_auto_save_period_labels[11] = {
  "Auto-save OFF",     "Auto-save 30 sec",  "Auto-save 60 sec",  "Auto-save 90 sec",
  "Auto-save 120 sec", "Auto-save 150 sec", "Auto-save 180 sec", "Auto-save 210 sec",
  "Auto-save 240 sec", "Auto-save 270 sec", "Auto-save 300 sec"
};

static void imp_popup(const char* text, float time)
{
  imp.popup_text = text;
  imp.popup_time = imp.perf_freq * time;
}

static void imp_update_frame_time()
{
  double freq = 0.0;
  SDL_DisplayID display = SDL_GetDisplayForWindow(imp.window);
  if (display) {
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    if (mode) {
      freq = mode->refresh_rate;
    }
  }
  imp.target_frame_time = imp.perf_freq / mn_video_freq(freq);
}

static void imp_ondisplaychange()
{
  SDL_GetWindowSizeInPixels(imp.window, &imp.width, &imp.height);
  mn_video_fit(imp.auto_aspect, imp.overscan, imp.width, imp.height, &imp.rect.x, &imp.rect.y, &imp.rect.w,
               &imp.rect.h);
  imp.ui_offset.w = imp.rect.w / 256;
  imp.ui_offset.h = imp.rect.h / 240;
  imp.ui_offset.x = imp.rect.x / imp.ui_offset.w;
  imp.ui_offset.y = imp.rect.y / imp.ui_offset.h;

  float ph = (imp.height < imp.width ? imp.height : imp.width) / 16.0f;
  uint ab_off = (float)imp.width / imp.height > 16.0 / 15.0 ? 1 : 0;

  imp.touch_button_radius = ph;
  imp.touch_button_ab_radius = 2 * ph;
  imp.touch_a_x = imp.width - (2 + ab_off) * ph;
  imp.touch_a_y = imp.height - 4 * ph;
  imp.touch_b_x = imp.width - (6 + ab_off) * ph;
  imp.touch_b_y = imp.height - 4 * ph;
  imp.touch_start_x = imp.width - 3 * ph;
  imp.touch_start_y = 5 * ph;
  imp.touch_select_x = 3 * ph;
  imp.touch_select_y = 5 * ph;
  imp.touch_menu_x = imp.width - 3 * ph;
  imp.touch_menu_y = 2 * ph;
  imp.touch_ff_x = 3 * ph;
  imp.touch_ff_y = 2 * ph;
  imp.touch_dpad_radius = 4 * ph;
  imp.touch_dpad_threshold = ph;
  imp.touch_dpad_x = 4 * ph;
  imp.touch_dpad_y = imp.height - 4 * ph;

  imp_update_frame_time();
}

static void imp_try_resize()
{
#ifndef __EMSCRIPTEN__
  uint width;
  uint height;
  mn_video_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);
  SDL_SetWindowSize(imp.window, width, height);
#endif
  imp_ondisplaychange();
}

static void imp_volume_update() { SDL_SetAudioStreamGain(imp.stream, (float)imp.volume / 10.0f); }

static void imp_ensure_dir(const char* subdir)
{
  char path[IMP_PATH_SIZE] = {};
  SDL_strlcat(path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(path, subdir, IMP_PATH_SIZE);
  SDL_CreateDirectory(path);
}

static const char* imp_rom_name(const char* path)
{
  const char* name = path;
  const char* slash = SDL_strrchr(path, '/');
  const char* backslash = SDL_strrchr(path, '\\');
  if (slash || backslash) {
    name = (slash > backslash ? slash : backslash) + 1;
  }
  return name;
}

static void imp_save_path(char* dst, const char* subdir, const char* ext)
{
  dst[0] = 0;
  SDL_snprintf(dst, IMP_PATH_SIZE, "%s%s/%s", imp.save_dir, subdir, imp_rom_name(imp.rom_path));
  char* dot = SDL_strrchr(dst, '.');
  if (dot) {
    *dot = 0;
  }
  SDL_strlcat(dst, ext, IMP_PATH_SIZE);
}

static void imp_load_history()
{
  if (imp.embed_enabled) {
    return;
  }
  for (uint i = 0; i < IMP_RECENT_COUNT; ++i) {
    imp.recent_files[i][0] = 0;
  }
  FILE* f = fopen(imp.history_path, "r");
  if (!f) {
    return;
  }
  for (uint i = 0; i < IMP_RECENT_COUNT; ++i) {
    if (!fscanf(f, "%[^\n]%*c", imp.recent_files[i])) {
      break;
    }
  }
  fclose(f);
}

static void imp_save_history()
{
  if (imp.embed_enabled) {
    return;
  }
  int idx = -1;
  for (int i = 0; i < IMP_RECENT_COUNT; ++i) {
    if (SDL_strcmp(imp.recent_files[i], imp.rom_path) == 0) {
      idx = i;
      break;
    }
  }
  if (idx == 0) {
    return;
  }
  if (idx == -1) {
    idx = IMP_RECENT_COUNT - 1;
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    if (imp.recent_files[idx][0]) {
      SDL_RemovePath(imp.recent_files[idx]);
    }
#endif
  }
  for (int i = idx; i > 0; --i) {
    SDL_strlcpy(imp.recent_files[i], imp.recent_files[i - 1], IMP_PATH_SIZE);
  }
  SDL_strlcpy(imp.recent_files[0], imp.rom_path, IMP_PATH_SIZE);
  FILE* f = fopen(imp.history_path, "w");
  if (!f) {
    return;
  }
  for (uint i = 0; i < IMP_RECENT_COUNT; ++i) {
    if (imp.recent_files[i][0] == 0) {
      break;
    }
    fprintf(f, "%s\n", imp.recent_files[i]);
  }
  fclose(f);
}

static void imp_rom_update()
{
  mn_rom rom = imp.rom;
  imp.menu_state = !rom ? IMP_MENU_MAIN : rom->mapper_error ? IMP_MENU_INFO : IMP_MENU_OFF;
  imp.pause = !rom || rom->mapper_error;
  if (!rom) {
    for (uint i = 0; i < 6; ++i) {
      imp.rom_info[i][0] = 0;
    }
    return;
  }

  if (!imp.embed_enabled) {
    char title[256] = {};
    SDL_strlcat(title, imp_rom_name(imp.rom_path), IMP_PATH_SIZE);
    SDL_strlcat(title, " - MyaowNES", IMP_PATH_SIZE);
    SDL_SetWindowTitle(imp.window, title);
  }

  SDL_memset(imp.rom_info, 0, sizeof(imp.rom_info));
  SDL_snprintf(imp.rom_info[0], sizeof(imp.rom_info[0]), "%-5s %1s%03d-%03d",
               rom->region == MN_REGION_NTSC    ? "NTSC"
               : rom->region == MN_REGION_PAL   ? "PAL"
               : rom->region == MN_REGION_AUTO  ? "Multi"
               : rom->region == MN_REGION_DENDY ? "Dendy"
                                                : "Error",
               rom->mapper_error ? "!" : " ", rom->mapper, rom->submapper);
  SDL_snprintf(imp.rom_info[1], sizeof(imp.rom_info[0]), "%-5s %-3s %4s", rom->vert_mirror ? "Vert" : "Horiz",
               rom->alt_mirror ? "Alt" : "", rom->has_trainer ? "TR" : "");
  SDL_snprintf(imp.rom_info[2], sizeof(imp.rom_info[0]), "PRG-ROM %5dK", rom->prg_rom_size >> 10);
  SDL_snprintf(imp.rom_info[3], sizeof(imp.rom_info[0]), "%-8s %4d%1s",
               rom->prg_has_battery ? (rom->prg_ram_size < 1024 ? "EEPROM" : "PRG-SRAM") : "PRG-RAM",
               rom->prg_ram_size < 1024 ? rom->prg_ram_size : rom->prg_ram_size >> 10,
               (rom->prg_ram_size > 0 && rom->prg_ram_size < 1024) ? "" : "K");
  SDL_snprintf(imp.rom_info[4], sizeof(imp.rom_info[0]), "CHR-ROM %5dK", rom->chr_rom_size >> 10);
  SDL_snprintf(imp.rom_info[5], sizeof(imp.rom_info[0]), "CHR-%-4s %4dK", rom->chr_has_battery ? "SRAM" : "RAM",
               rom->chr_ram_size >> 10);

  SDL_Log("Rom: %s", imp.rom_path);
  for (uint i = 0; i < 6; ++i) {
    SDL_Log("     %s", imp.rom_info[i]);
  }

  imp_try_resize();
  imp_save_history();
}

static void imp_tas_unload()
{
  if (imp.tas) {
    mn_tas_release(imp.tas);
    imp.tas = nullptr;
  }
}

static void imp_tas_load(const char* path)
{
  imp_tas_unload();
  imp.tas = mn_tas_load(path);
  if (imp.tas) {
    mn_power();
  }
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
  imp_tas_unload();
  imp_rom_unload();
  SDL_strlcpy(imp.rom_path, path, IMP_PATH_SIZE);
  imp_save_path(imp.sram_path, "Saves", ".sav");
  imp.rom = mn_rom_load(imp.rom_path, imp.sram_path);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
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
  fprintf(f, "volume %u\n", imp.volume);
  fprintf(f, "scale %u\n", imp.scale);
  fprintf(f, "auto_aspect %u\n", imp.auto_aspect);
  fprintf(f, "overscan %u\n", imp.overscan);
  fprintf(f, "auto_save_period %u\n", imp.auto_save_period);
  fprintf(f, "show_fps %u\n", imp.fps);
  fprintf(f, "fast_forward_scale %u\n", imp.fast_forward_scale);
  fprintf(f, "region %u\n", imp.region);
#ifndef __EMSCRIPTEN__
  // fprintf(f, "filter_hp %u\n", imp.filter_hp);
  // fprintf(f, "filter_lp %u\n", imp.filter_lp);
  fprintf(f, "palette \"%s\"\n", imp.palette_path);
#endif
  for (uint i = 1; i < IMP_ACTION_COUNT; ++i) {
    fprintf(f, "%s \"%s\"\n", imp_actions[i].label, SDL_GetScancodeName(imp.scancode_by_action[i]));
  }
  fclose(f);
  imp.dirty_config = false;
}

static void imp_read_config(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f) {
    return;
  }

  char key[32];
  while (fscanf(f, " %31s", key) == 1) {
    if (strcmp(key, "volume") == 0) {
      fscanf(f, "%u", &imp.volume);
    } else if (strcmp(key, "scale") == 0) {
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
    } else if (strcmp(key, "show_fps") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.fps = val;
    } else if (strcmp(key, "fast_forward_scale") == 0) {
      fscanf(f, "%u", &imp.fast_forward_scale);
    } else if (strcmp(key, "region") == 0) {
      fscanf(f, "%u", &imp.region);
      // } else if (strcmp(key, "filter_hp") == 0) {
      //   fscanf(f, "%u", &imp.filter_hp);
      // } else if (strcmp(key, "filter_lp") == 0) {
      //   fscanf(f, "%u", &imp.filter_lp);
    } else if (strcmp(key, "palette") == 0) {
      fscanf(f, " \"%[^\"]\"", imp.palette_path);
      if (imp.palette_path[0]) {
        size_t size = 0;
        u8* data = SDL_LoadFile(imp.palette_path, &size);
        if (data && size == 192) {
          uint palette[64] = {};
          for (uint i = 0; i < 64; ++i) {
            u8 r = data[i * 3 + 0];
            u8 g = data[i * 3 + 1];
            u8 b = data[i * 3 + 2];
            palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
            // SDL_Log("0x%08X,", palette[i]);
          }
          mn_palette(palette);
        }
        SDL_free(data);
      }
    } else if (strcmp(key, "start_screen") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.start_screen = val;
    } else {
      for (uint i = 1; i < IMP_ACTION_COUNT; ++i) {
        if (strcmp(key, imp_actions[i].label) == 0) {
          char val[32];
          fscanf(f, " \"%[^\"]\"", val);
          imp.scancode_by_action[i] = SDL_GetScancodeFromName(val);
        }
      }
    }
  }
  fclose(f);
}

static void imp_config_update()
{
  memset(imp.action_by_scancode, 0, sizeof(imp.action_by_scancode));
  for (uint i = 0; i < IMP_ACTION_COUNT; ++i) {
    imp.action_by_scancode[imp.scancode_by_action[i]] = i;
  }
  // mn_audio_hpf(imp.filter_hp);
  // mn_audio_lpf(imp.filter_lp);
}

static void imp_config_defaults()
{
  imp.start_screen = 1;
  imp.volume = 10;
  imp.scale = 2;
  imp.auto_aspect = true;
  imp.overscan = false;
  imp.auto_save_period = 60;
  imp.fps = false;
  imp.fps_test = false;
  imp.fast_forward_scale = 4;
  imp.region = MN_REGION_AUTO;
  // imp.filter_hp = 0;
  // imp.filter_lp = 0;
  imp.scancode_by_action[IMP_ACTION_JOY1_A] = SDL_SCANCODE_K;
  imp.scancode_by_action[IMP_ACTION_JOY1_B] = SDL_SCANCODE_J;
  imp.scancode_by_action[IMP_ACTION_JOY1_A_TURBO] = SDL_SCANCODE_I;
  imp.scancode_by_action[IMP_ACTION_JOY1_B_TURBO] = SDL_SCANCODE_U;
  imp.scancode_by_action[IMP_ACTION_JOY1_START] = SDL_SCANCODE_H;
  imp.scancode_by_action[IMP_ACTION_JOY1_SELECT] = SDL_SCANCODE_F;
  imp.scancode_by_action[IMP_ACTION_JOY1_UP] = SDL_SCANCODE_W;
  imp.scancode_by_action[IMP_ACTION_JOY1_DOWN] = SDL_SCANCODE_S;
  imp.scancode_by_action[IMP_ACTION_JOY1_LEFT] = SDL_SCANCODE_A;
  imp.scancode_by_action[IMP_ACTION_JOY1_RIGHT] = SDL_SCANCODE_D;
  imp.scancode_by_action[IMP_ACTION_JOY2_A] = SDL_SCANCODE_KP_2;
  imp.scancode_by_action[IMP_ACTION_JOY2_B] = SDL_SCANCODE_KP_1;
  imp.scancode_by_action[IMP_ACTION_JOY2_A_TURBO] = SDL_SCANCODE_KP_5;
  imp.scancode_by_action[IMP_ACTION_JOY2_B_TURBO] = SDL_SCANCODE_KP_4;
  imp.scancode_by_action[IMP_ACTION_JOY2_START] = SDL_SCANCODE_RETURN;
  imp.scancode_by_action[IMP_ACTION_JOY2_SELECT] = SDL_SCANCODE_SPACE;
  imp.scancode_by_action[IMP_ACTION_JOY2_UP] = SDL_SCANCODE_UP;
  imp.scancode_by_action[IMP_ACTION_JOY2_DOWN] = SDL_SCANCODE_DOWN;
  imp.scancode_by_action[IMP_ACTION_JOY2_LEFT] = SDL_SCANCODE_LEFT;
  imp.scancode_by_action[IMP_ACTION_JOY2_RIGHT] = SDL_SCANCODE_RIGHT;
  imp.scancode_by_action[IMP_ACTION_MENU] = SDL_SCANCODE_ESCAPE;
  imp.scancode_by_action[IMP_ACTION_HELP] = SDL_SCANCODE_F1;
  imp.scancode_by_action[IMP_ACTION_OPEN] = SDL_SCANCODE_F2;
  imp.scancode_by_action[IMP_ACTION_REGION] = SDL_SCANCODE_F3;
  imp.scancode_by_action[IMP_ACTION_RESET] = SDL_SCANCODE_F4;
  imp.scancode_by_action[IMP_ACTION_SAVE] = SDL_SCANCODE_F5;
  imp.scancode_by_action[IMP_ACTION_ASPECT] = SDL_SCANCODE_F6;
  imp.scancode_by_action[IMP_ACTION_OVERSCAN] = SDL_SCANCODE_F7;
  imp.scancode_by_action[IMP_ACTION_LOAD] = SDL_SCANCODE_F8;
  imp.scancode_by_action[IMP_ACTION_QUIT] = SDL_SCANCODE_F10;
  imp.scancode_by_action[IMP_ACTION_FULLSCREEN] = SDL_SCANCODE_F11;
  imp.scancode_by_action[IMP_ACTION_SCREENSHOT] = SDL_SCANCODE_F12;
  imp.scancode_by_action[IMP_ACTION_PAUSE] = SDL_SCANCODE_PAUSE;
  imp.scancode_by_action[IMP_ACTION_SHOW_FPS] = SDL_SCANCODE_GRAVE;
  imp.scancode_by_action[IMP_ACTION_MAX_FPS] = SDL_SCANCODE_Z;
  imp.scancode_by_action[IMP_ACTION_FAST_FORWARD] = SDL_SCANCODE_TAB;
  imp.scancode_by_action[IMP_ACTION_SLOT0] = SDL_SCANCODE_0;
  imp.scancode_by_action[IMP_ACTION_SLOT1] = SDL_SCANCODE_1;
  imp.scancode_by_action[IMP_ACTION_SLOT2] = SDL_SCANCODE_2;
  imp.scancode_by_action[IMP_ACTION_SLOT3] = SDL_SCANCODE_3;
  imp.scancode_by_action[IMP_ACTION_SLOT4] = SDL_SCANCODE_4;
  imp.scancode_by_action[IMP_ACTION_SLOT5] = SDL_SCANCODE_5;
  imp.scancode_by_action[IMP_ACTION_SLOT6] = SDL_SCANCODE_6;
  imp.scancode_by_action[IMP_ACTION_SLOT7] = SDL_SCANCODE_7;
  imp.scancode_by_action[IMP_ACTION_SLOT8] = SDL_SCANCODE_8;
  imp.scancode_by_action[IMP_ACTION_SLOT9] = SDL_SCANCODE_9;
  imp.scancode_by_action[IMP_ACTION_SLOT_NEXT] = SDL_SCANCODE_UNKNOWN;
  imp.scancode_by_action[IMP_ACTION_SLOT_PREV] = SDL_SCANCODE_UNKNOWN;
  imp.scancode_by_action[IMP_ACTION_VOLUME_UP] = SDL_SCANCODE_EQUALS;
  imp.scancode_by_action[IMP_ACTION_VOLUME_DOWN] = SDL_SCANCODE_MINUS;
  imp.scancode_by_action[IMP_ACTION_SCALE_UP] = SDL_SCANCODE_RIGHTBRACKET;
  imp.scancode_by_action[IMP_ACTION_SCALE_DOWN] = SDL_SCANCODE_LEFTBRACKET;

  if (imp.default_config_path[0]) {
    imp_read_config(imp.default_config_path);
  }
}

static void imp_load_config()
{
  imp_config_defaults();
  imp_read_config(imp.config_path);
  imp_config_update();
}

static void imp_reset_config()
{
  uint old_region = imp.region;
  imp_config_defaults();
  imp_config_update();
  imp.dirty_config = true;
  if (imp.region != old_region) {
    mn_region_set(imp.region);
    if (imp.rom) {
      mn_rom_set(imp.rom);
    }
  }
  imp_try_resize();
}

static bool imp_touch_hit(float ex, float ey, int x, int y, int r)
{
  int dx = ex - x;
  int dy = ey - y;
  return (dx * dx + dy * dy) < (r * r);
}

bool imp_audio_suspended()
{
#ifdef __EMSCRIPTEN__
  return emscripten_run_script_int(
    "(function() {"
    "  var ctx = new AudioContext();"
    "  var state = ctx.state;"
    "  ctx.close();"
    "  return (state === 'suspended') ? 1 : 0;"
    "})()");
#else
  return false;
#endif
}

#include "app.h"
#include <stdio.h>

void app_ondisplaychange()
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

  SDL_GetWindowSizeInPixels(imp.window, &imp.width, &imp.height);
  int x = 0;
  int y = 0;
  uint w = 0;
  uint h = 0;
  mn_video_fit(imp.auto_aspect, imp.overscan, imp.width, imp.height, &x, &y, &w, &h);
  imp.rect.x = x;
  imp.rect.y = y;
  imp.rect.w = w;
  imp.rect.h = h;
  imp.ui_offset.w = (float)w / 256;
  imp.ui_offset.h = (float)h / 240;
  imp.ui_offset.x = (float)x / imp.ui_offset.w;
  imp.ui_offset.y = (float)y / imp.ui_offset.h;

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
}

void app_try_resize()
{
#ifndef __EMSCRIPTEN__
  uint width;
  uint height;
  mn_video_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);
  SDL_SetWindowSize(imp.window, width, height);
#endif
  app_ondisplaychange();
}

void app_volume_update() { SDL_SetAudioStreamGain(imp.stream, (float)imp.volume / 10.0f); }

const char* app_rom_name(const char* path)
{
  const char* name = path;
  const char* slash = SDL_strrchr(path, '/');
  const char* backslash = SDL_strrchr(path, '\\');
  if (slash || backslash) {
    name = (slash > backslash ? slash : backslash) + 1;
  }
  return name;
}

void app_save_path(char* dst, const char* subdir, const char* ext)
{
  dst[0] = 0;
  SDL_snprintf(dst, APP_PATH_SIZE, "%s%s/%s", imp.save_dir, subdir, app_rom_name(imp.rom_path));
  char* dot = SDL_strrchr(dst, '.');
  if (dot) {
    *dot = 0;
  }
  SDL_strlcat(dst, ext, APP_PATH_SIZE);
}

void app_load_history()
{
  if (imp.embed_enabled) {
    return;
  }
  for (uint i = 0; i < APP_RECENT_COUNT; ++i) {
    imp.recent_files[i][0] = 0;
  }
  FILE* f = fopen(imp.history_path, "r");
  if (!f) {
    return;
  }
  for (uint i = 0; i < APP_RECENT_COUNT; ++i) {
    if (!fscanf(f, "%[^\n]%*c", imp.recent_files[i])) {
      break;
    }
  }
  fclose(f);
}

static void app_save_history()
{
  if (imp.embed_enabled) {
    return;
  }
  int idx = -1;
  for (int i = 0; i < APP_RECENT_COUNT; ++i) {
    if (SDL_strcmp(imp.recent_files[i], imp.rom_path) == 0) {
      idx = i;
      break;
    }
  }
  if (idx == 0) {
    return;
  }
  if (idx == -1) {
    idx = APP_RECENT_COUNT - 1;
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    if (imp.recent_files[idx][0]) {
      SDL_RemovePath(imp.recent_files[idx]);
    }
#endif
  }
  for (int i = idx; i > 0; --i) {
    SDL_strlcpy(imp.recent_files[i], imp.recent_files[i - 1], APP_PATH_SIZE);
  }
  SDL_strlcpy(imp.recent_files[0], imp.rom_path, APP_PATH_SIZE);
  FILE* f = fopen(imp.history_path, "w");
  if (!f) {
    return;
  }
  for (uint i = 0; i < APP_RECENT_COUNT; ++i) {
    if (imp.recent_files[i][0] == 0) {
      break;
    }
    fprintf(f, "%s\n", imp.recent_files[i]);
  }
  fclose(f);
}

void app_rom_update()
{
  mn_rom rom = imp.rom;
  imp.menu_state = !rom ? APP_MENU_MAIN : rom->mapper_error ? APP_MENU_INFO : APP_MENU_OFF;
  imp.pause = !rom || rom->mapper_error;
  if (!rom) {
    for (uint i = 0; i < 6; ++i) {
      imp.rom_info[i][0] = 0;
    }
    return;
  }

  if (!imp.embed_enabled) {
    char title[256] = {};
    SDL_strlcat(title, app_rom_name(imp.rom_path), APP_PATH_SIZE);
    SDL_strlcat(title, " - MyaowNES", APP_PATH_SIZE);
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

  app_try_resize();
  app_save_history();
}

void app_tas_unload()
{
  if (imp.tas) {
    mn_tas_release(imp.tas);
    imp.tas = nullptr;
  }
}

void app_tas_load(const char* path)
{
  app_tas_unload();
  imp.tas = mn_tas_load(path);
  if (imp.tas) {
    mn_power();
  }
}

void app_rom_unload()
{
  if (imp.rom) {
    if ((imp.rom->prg_has_battery || imp.rom->chr_has_battery)) {
      mn_rom_save(imp.sram_path);
    }
    mn_rom_release(imp.rom);
  }
}

void app_rom_load(const char* path)
{
  app_tas_unload();
  app_rom_unload();
  SDL_strlcpy(imp.rom_path, path, APP_PATH_SIZE);
  app_save_path(imp.sram_path, "Saves", ".sav");
  imp.rom = mn_rom_load(imp.rom_path, imp.sram_path);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
}

void app_save_config()
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
  fprintf(f, "atlas \"%s\"\n", imp.atlas_path);
#endif
  for (uint i = 1; i < APP_ACTION_COUNT; ++i) {
    fprintf(f, "%s \"%s\"\n", app_actions[i].label, SDL_GetScancodeName(imp.scancode_by_action[i]));
  }
  fclose(f);
  imp.dirty_config = false;
}

static void app_read_config(const char* path)
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
    } else if (strcmp(key, "atlas") == 0) {
      fscanf(f, " \"%[^\"]\"", imp.atlas_path);
    } else if (strcmp(key, "start_screen") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.start_screen = val;
    } else {
      for (uint i = 1; i < APP_ACTION_COUNT; ++i) {
        if (strcmp(key, app_actions[i].label) == 0) {
          char val[32];
          fscanf(f, " \"%[^\"]\"", val);
          imp.scancode_by_action[i] = SDL_GetScancodeFromName(val);
        }
      }
    }
  }
  fclose(f);
}

static void app_config_update()
{
  memset(imp.action_by_scancode, 0, sizeof(imp.action_by_scancode));
  for (uint i = 0; i < APP_ACTION_COUNT; ++i) {
    imp.action_by_scancode[imp.scancode_by_action[i]] = i;
  }
  // mn_audio_hpf(imp.filter_hp);
  // mn_audio_lpf(imp.filter_lp);
}

static void app_config_defaults()
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
  imp.scancode_by_action[APP_ACTION_JOY1_A] = SDL_SCANCODE_K;
  imp.scancode_by_action[APP_ACTION_JOY1_B] = SDL_SCANCODE_J;
  imp.scancode_by_action[APP_ACTION_JOY1_A_TURBO] = SDL_SCANCODE_I;
  imp.scancode_by_action[APP_ACTION_JOY1_B_TURBO] = SDL_SCANCODE_U;
  imp.scancode_by_action[APP_ACTION_JOY1_START] = SDL_SCANCODE_H;
  imp.scancode_by_action[APP_ACTION_JOY1_SELECT] = SDL_SCANCODE_F;
  imp.scancode_by_action[APP_ACTION_JOY1_UP] = SDL_SCANCODE_W;
  imp.scancode_by_action[APP_ACTION_JOY1_DOWN] = SDL_SCANCODE_S;
  imp.scancode_by_action[APP_ACTION_JOY1_LEFT] = SDL_SCANCODE_A;
  imp.scancode_by_action[APP_ACTION_JOY1_RIGHT] = SDL_SCANCODE_D;
  imp.scancode_by_action[APP_ACTION_JOY2_A] = SDL_SCANCODE_KP_2;
  imp.scancode_by_action[APP_ACTION_JOY2_B] = SDL_SCANCODE_KP_1;
  imp.scancode_by_action[APP_ACTION_JOY2_A_TURBO] = SDL_SCANCODE_KP_5;
  imp.scancode_by_action[APP_ACTION_JOY2_B_TURBO] = SDL_SCANCODE_KP_4;
  imp.scancode_by_action[APP_ACTION_JOY2_START] = SDL_SCANCODE_RETURN;
  imp.scancode_by_action[APP_ACTION_JOY2_SELECT] = SDL_SCANCODE_SPACE;
  imp.scancode_by_action[APP_ACTION_JOY2_UP] = SDL_SCANCODE_UP;
  imp.scancode_by_action[APP_ACTION_JOY2_DOWN] = SDL_SCANCODE_DOWN;
  imp.scancode_by_action[APP_ACTION_JOY2_LEFT] = SDL_SCANCODE_LEFT;
  imp.scancode_by_action[APP_ACTION_JOY2_RIGHT] = SDL_SCANCODE_RIGHT;
  imp.scancode_by_action[APP_ACTION_MENU] = SDL_SCANCODE_ESCAPE;
  imp.scancode_by_action[APP_ACTION_HELP] = SDL_SCANCODE_F1;
  imp.scancode_by_action[APP_ACTION_OPEN] = SDL_SCANCODE_F2;
  imp.scancode_by_action[APP_ACTION_REGION] = SDL_SCANCODE_F3;
  imp.scancode_by_action[APP_ACTION_RESET] = SDL_SCANCODE_F4;
  imp.scancode_by_action[APP_ACTION_SAVE] = SDL_SCANCODE_F5;
  imp.scancode_by_action[APP_ACTION_ASPECT] = SDL_SCANCODE_F6;
  imp.scancode_by_action[APP_ACTION_OVERSCAN] = SDL_SCANCODE_F7;
  imp.scancode_by_action[APP_ACTION_LOAD] = SDL_SCANCODE_F8;
  imp.scancode_by_action[APP_ACTION_QUIT] = SDL_SCANCODE_F10;
  imp.scancode_by_action[APP_ACTION_FULLSCREEN] = SDL_SCANCODE_F11;
  imp.scancode_by_action[APP_ACTION_SCREENSHOT] = SDL_SCANCODE_F12;
  imp.scancode_by_action[APP_ACTION_PAUSE] = SDL_SCANCODE_PAUSE;
  imp.scancode_by_action[APP_ACTION_SHOW_FPS] = SDL_SCANCODE_GRAVE;
  imp.scancode_by_action[APP_ACTION_MAX_FPS] = SDL_SCANCODE_Z;
  imp.scancode_by_action[APP_ACTION_FAST_FORWARD] = SDL_SCANCODE_TAB;
  imp.scancode_by_action[APP_ACTION_SLOT0] = SDL_SCANCODE_0;
  imp.scancode_by_action[APP_ACTION_SLOT1] = SDL_SCANCODE_1;
  imp.scancode_by_action[APP_ACTION_SLOT2] = SDL_SCANCODE_2;
  imp.scancode_by_action[APP_ACTION_SLOT3] = SDL_SCANCODE_3;
  imp.scancode_by_action[APP_ACTION_SLOT4] = SDL_SCANCODE_4;
  imp.scancode_by_action[APP_ACTION_SLOT5] = SDL_SCANCODE_5;
  imp.scancode_by_action[APP_ACTION_SLOT6] = SDL_SCANCODE_6;
  imp.scancode_by_action[APP_ACTION_SLOT7] = SDL_SCANCODE_7;
  imp.scancode_by_action[APP_ACTION_SLOT8] = SDL_SCANCODE_8;
  imp.scancode_by_action[APP_ACTION_SLOT9] = SDL_SCANCODE_9;
  imp.scancode_by_action[APP_ACTION_SLOT_NEXT] = SDL_SCANCODE_UNKNOWN;
  imp.scancode_by_action[APP_ACTION_SLOT_PREV] = SDL_SCANCODE_UNKNOWN;
  imp.scancode_by_action[APP_ACTION_VOLUME_UP] = SDL_SCANCODE_EQUALS;
  imp.scancode_by_action[APP_ACTION_VOLUME_DOWN] = SDL_SCANCODE_MINUS;
  imp.scancode_by_action[APP_ACTION_SCALE_UP] = SDL_SCANCODE_RIGHTBRACKET;
  imp.scancode_by_action[APP_ACTION_SCALE_DOWN] = SDL_SCANCODE_LEFTBRACKET;

  if (imp.default_config_path[0]) {
    app_read_config(imp.default_config_path);
  }
}

void app_load_config()
{
  app_config_defaults();
  app_read_config(imp.config_path);
  app_config_update();
}

void app_reset_config()
{
  uint old_region = imp.region;
  app_config_defaults();
  app_config_update();
  imp.dirty_config = true;
  if (imp.region != old_region) {
    mn_region_set(imp.region);
    if (imp.rom) {
      mn_rom_set(imp.rom);
    }
  }
  app_try_resize();
}

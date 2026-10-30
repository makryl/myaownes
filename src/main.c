#include "myaownes.h"
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void imp_rom_load(const char* path);
static void imp_tas_load(const char* path);

static bool imp_init()
{
  const char* rom = nullptr;
  const char* tas = nullptr;

  // rom = "../../external/AccuracyCoin/AccuracyCoin.nes";
  // rom = "../../external/nes-test-roms/CPU/nestest/nestest.nes";
  // rom = "../../external/nes-test-roms/Interactive/240pee/240pee.nes";
  // rom = "../../external/nes-test-roms/Input/allpads.nes";
  // rom = "../../external/nes-test-roms/PPU/ppu_vbl_nmi/ppu_vbl_nmi.nes";
  // rom = "../../external/nes-test-roms/PPU/nmi_sync/demo_ntsc.nes";
  // rom = "../../external/nes-test-roms/PPU/nmi_sync/demo_pal.nes";
  // rom = "../../external/nes-test-roms/PPU/ppu_sprite_hit/rom_singles/09-timing.nes";
  // rom = "../../external/nes-test-roms/PPU/ppu_sprite_overflow/rom_singles/03-timing.nes";
  // rom = "../../external/nes-test-roms/APU/apu_test/rom_singles/7-dmc_basics.nes";
  // rom = "../../external/nes-test-roms/APU/apu_test/rom_singles/8-dmc_rates.nes";
  // rom = "../../external/nes-test-roms/Interactive/tvpassfail/tv.nes"; // todo: chroma/luma?
  // rom = "../../external/nes-test-roms/Demos/8bitpeoples_-_deadline_console_invitro.nes";
  // rom = "../../external/nes-test-roms/Mappers/n163test.nes"; // todo fail audio
  // rom = "../../external/nes-test-roms/PPU/sprite_evaluation_test(2).nes"; // todo
  // rom = "../../external/nes-test-roms/PPU/sprite_evaluation_test.nes"; // todo
  // rom = "../../external/nes-test-roms/CPU/cpu_flag_concurrency/test_cpu_flag_concurrency.nes"; // 4015 open bus bit?
  // rom = "../../external/nes-test-roms/Mappers/mmc5test_v2/mmc5test.nes"; // todo

  // rom = "../../tmp/MegaPack/USA/Super Mario Bros 3 (U) (PRG 1).nes";
  // tas = "../../tmp/tas/lordtom-smb3-totalcontrol.fm2";

  // rom = "../../tmp/GoodNES/USA/Battletoads (U) [!].nes";
  // tas = "../../tmp/tas/feos,alyosha,samsarav1-battletoads.fm2";

  if (rom) {
    imp_rom_load(rom);
  }
  if (tas) {
    imp_tas_load(tas);
  }

  return true;
}

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

static void imp_draw_popup()
{
  imp.popup_time -= imp.popup_time > imp.real_frame_time ? imp.real_frame_time : imp.popup_time;
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 2, imp.ui_offset.y + 21, imp.popup_text);
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 1, imp.ui_offset.y + 20, imp.popup_text);
}

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

static void imp_reset()
{
  if (imp.tas) {
    mn_tas_reset(imp.tas);
  }
  mn_reset();
}

static void imp_save()
{
  char qs_path[IMP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  imp_save_path(qs_path, "Saves", ext);
  if (mn_save(qs_path)) {
    imp_popup("Saved", 2);
  }
}

static bool imp_load_check()
{
  char qs_path[IMP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  imp_save_path(qs_path, "Saves", ext);
  bool result = mn_load(qs_path);
  if (result) {
    imp_popup("Loaded", 2);
  }
  return result;
}

static void imp_load() { imp_load_check(); }

static void imp_menu_state(uint state)
{
  if (state == IMP_MENU_OFF && !imp.rom) {
    state = IMP_MENU_MAIN;
  }
  if (state == imp.menu_state) {
    return;
  }
  if (imp.menu_state != IMP_MENU_HELP) {
    imp.menu_state_prev = imp.menu_state;
  }
  imp.menu_state = state;
  imp.menu_selection = 0;
  imp.pause = state != IMP_MENU_OFF;
}

static void imp_toggle_menu() { imp_menu_state(imp.menu_state == IMP_MENU_MAIN ? IMP_MENU_OFF : IMP_MENU_MAIN); }
static void imp_toggle_help() { imp_menu_state(imp.menu_state == IMP_MENU_HELP ? imp.menu_state_prev : IMP_MENU_HELP); }

static void imp_toggle_pause()
{
  if (imp.menu_state == IMP_MENU_OFF) {
    imp.pause = !imp.pause;
    if (imp.pause) {
      imp_popup("Pause", 2);
    }
  }
}

static void imp_copy_tmp_file(const char* src, char* dst)
{
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
  SDL_snprintf(dst, IMP_PATH_SIZE, "%s%s/%s", imp.save_dir, "Roms", imp_rom_name(src));
  SDL_CopyFile(src, dst);
#else
  SDL_strlcpy(dst, src, IMP_PATH_SIZE);
#endif
}

static void SDLCALL imp_file_dialog_cb(void*, const char* const* files, int)
{
  if (files && files[0]) {
    imp_copy_tmp_file(files[0], imp.load_rom_path);
  } else {
    imp.load_rom_path[0] = 0;
  }
  SDL_SetAtomicInt(&imp.load_rom_dialog_done, 1);
}

static void imp_open_file()
{
  if (imp.load_rom_dialog_wait || imp.embed_enabled) {
    return;
  }
  imp.load_rom_dialog_wait = true;
  SDL_DialogFileFilter filters[] = { { "iNES / NES 2.0 ROMs (*.nes)", "nes" }, { "All Files (*.*)", "*" } };
  SDL_ShowOpenFileDialog(imp_file_dialog_cb, nullptr, imp.window, filters, 2, nullptr, false);
}

static void imp_toggle_region()
{
  imp.region = (imp.region + 1) % 4;
  imp.dirty_config = true;
  mn_region_set(imp.region);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
  imp_try_resize();
  imp_popup(imp_region_labels[imp.region], 2);
}

static void imp_toggle_aspect()
{
  imp.auto_aspect = !imp.auto_aspect;
  imp.dirty_config = true;
  imp_try_resize();
  imp_popup(imp_aspect_labels[imp.auto_aspect], 2);
}

static void imp_toggle_overscan()
{
  imp.overscan = !imp.overscan;
  imp.dirty_config = true;
  imp_try_resize();
  imp_popup(imp_overscan_labels[imp.overscan], 2);
}

static void imp_set_fullscreen(bool enabled)
{
  SDL_SetWindowFullscreen(imp.window, enabled);
  imp_ondisplaychange();
  imp_popup(imp_fullscreen_labels[enabled], 2);
}

static void imp_toggle_fullscreen()
{
  Uint32 flags = SDL_GetWindowFlags(imp.window);
  imp_set_fullscreen((flags & SDL_WINDOW_FULLSCREEN) == 0);
}

static void imp_screenshot()
{
#ifndef __EMSCRIPTEN__
  SDL_Surface* screenshot = SDL_RenderReadPixels(imp.renderer, nullptr);
  if (screenshot) {
    char path[IMP_PATH_SIZE] = {};
    char ext[32] = "";
    uint num = 0;
    SDL_PathInfo info;
    while (true) {
      SDL_snprintf(ext, sizeof(ext), ".%d.png", num);
      imp_save_path(path, "Screenshots", ext);
      if (!SDL_GetPathInfo(path, &info)) {
        break;
      }
      ++num;
    }
    SDL_SavePNG(screenshot, path);
    SDL_DestroySurface(screenshot);
  }
#endif
}

static void imp_volume_down()
{
  if (imp.volume > 0) {
    --imp.volume;
    imp.dirty_config = true;
  }
  imp_popup(imp_volume_labels[imp.volume], 2);
  imp_volume_update();
}

static void imp_volume_up()
{
  if (imp.volume < 10) {
    ++imp.volume;
    imp.dirty_config = true;
  }
  imp_popup(imp_volume_labels[imp.volume], 2);
  imp_volume_update();
}

static void imp_upscale()
{
#ifndef __EMSCRIPTEN__
  if (imp.scale < 8) {
    ++imp.scale;
    imp.dirty_config = true;
  }
  imp_popup(imp_scale_labels[imp.scale - 1], 2);
  imp_try_resize();
#endif
}

static void imp_downscale()
{
#ifndef __EMSCRIPTEN__
  if (imp.scale > 1) {
    --imp.scale;
    imp.dirty_config = true;
  }
  imp_popup(imp_scale_labels[imp.scale - 1], 2);
  imp_try_resize();
#endif
}

static void imp_fast_forward_on()
{
  imp.fast_forward = true;
  SDL_SetAudioStreamFrequencyRatio(imp.stream, imp.fast_forward_scale);
}

static void imp_fast_forward_off()
{
  imp.fast_forward = false;
  SDL_SetAudioStreamFrequencyRatio(imp.stream, 1.0f);
}

static void imp_fast_forward_down()
{
  if (imp.fast_forward_scale > 2) {
    --imp.fast_forward_scale;
    imp.dirty_config = true;
  }
}

static void imp_fast_forward_up()
{
  if (imp.fast_forward_scale < 5) {
    ++imp.fast_forward_scale;
    imp.dirty_config = true;
  }
}

static void imp_auto_save_period_down()
{
  imp.auto_save_period = (imp.auto_save_period / 30) * 30;
  if (imp.auto_save_period > 0) {
    imp.auto_save_period -= 30;
    imp.dirty_config = true;
  }
}

static void imp_auto_save_period_up()
{
  imp.auto_save_period = (imp.auto_save_period / 30) * 30;
  if (imp.auto_save_period < 300) {
    imp.auto_save_period += 30;
    imp.dirty_config = true;
  }
}

static void imp_toggle_fps()
{
  imp.fps = !imp.fps;
  imp.dirty_config = true;
}

static void imp_toggle_fps_test()
{
#ifndef __EMSCRIPTEN__
  imp.fps_test = !imp.fps_test;
  SDL_SetRenderVSync(imp.renderer, !imp.fps_test);
  imp_popup(imp_fps_test_labels[imp.fps_test], 2);
#endif
}

static void imp_slot(uint slot)
{
  imp.slot = slot;
  imp_popup(imp_save_slot_labels[imp.slot], 2);
}

static void imp_slot0() { imp_slot(0); }
static void imp_slot1() { imp_slot(1); }
static void imp_slot2() { imp_slot(2); }
static void imp_slot3() { imp_slot(3); }
static void imp_slot4() { imp_slot(4); }
static void imp_slot5() { imp_slot(5); }
static void imp_slot6() { imp_slot(6); }
static void imp_slot7() { imp_slot(7); }
static void imp_slot8() { imp_slot(8); }
static void imp_slot9() { imp_slot(9); }

static void imp_slot_next()
{
  imp.slot = (imp.slot + 1) % 10;
  imp_popup(imp_save_slot_labels[imp.slot], 2);
}

static void imp_slot_prev()
{
  imp.slot = (imp.slot + 9) % 10;
  imp_popup(imp_save_slot_labels[imp.slot], 2);
}

static void imp_quit()
{
#ifndef __EMSCRIPTEN__
  imp.quit = true;
#endif
}

static void imp_joy1_dpad_up_down() { imp.joy1 |= MN_INPUT_UP; }
static void imp_joy1_dpad_up_up() { imp.joy1 &= ~MN_INPUT_UP; }
static void imp_joy1_dpad_left_down() { imp.joy1 |= MN_INPUT_LEFT; }
static void imp_joy1_dpad_left_up() { imp.joy1 &= ~MN_INPUT_LEFT; }
static void imp_joy1_dpad_down_down() { imp.joy1 |= MN_INPUT_DOWN; }
static void imp_joy1_dpad_down_up() { imp.joy1 &= ~MN_INPUT_DOWN; }
static void imp_joy1_dpad_right_down() { imp.joy1 |= MN_INPUT_RIGHT; }
static void imp_joy1_dpad_right_up() { imp.joy1 &= ~MN_INPUT_RIGHT; }
static void imp_joy1_a_down() { imp.joy1 |= MN_INPUT_A; }
static void imp_joy1_a_up() { imp.joy1 &= ~MN_INPUT_A; }
static void imp_joy1_b_down() { imp.joy1 |= MN_INPUT_B; }
static void imp_joy1_b_up() { imp.joy1 &= ~MN_INPUT_B; }
static void imp_joy1_a_turbo_down() { imp.joy1 |= MN_INPUT_TURBO_A; }
static void imp_joy1_a_turbo_up() { imp.joy1 &= ~MN_INPUT_TURBO_A; }
static void imp_joy1_b_turbo_down() { imp.joy1 |= MN_INPUT_TURBO_B; }
static void imp_joy1_b_turbo_up() { imp.joy1 &= ~MN_INPUT_TURBO_B; }
static void imp_joy1_select_down() { imp.joy1 |= MN_INPUT_SELECT; }
static void imp_joy1_select_up() { imp.joy1 &= ~MN_INPUT_SELECT; }
static void imp_joy1_start_down() { imp.joy1 |= MN_INPUT_START; }
static void imp_joy1_start_up() { imp.joy1 &= ~MN_INPUT_START; }

static void imp_joy2_dpad_up_down() { imp.joy2 |= MN_INPUT_UP; }
static void imp_joy2_dpad_up_up() { imp.joy2 &= ~MN_INPUT_UP; }
static void imp_joy2_dpad_left_down() { imp.joy2 |= MN_INPUT_LEFT; }
static void imp_joy2_dpad_left_up() { imp.joy2 &= ~MN_INPUT_LEFT; }
static void imp_joy2_dpad_down_down() { imp.joy2 |= MN_INPUT_DOWN; }
static void imp_joy2_dpad_down_up() { imp.joy2 &= ~MN_INPUT_DOWN; }
static void imp_joy2_dpad_right_down() { imp.joy2 |= MN_INPUT_RIGHT; }
static void imp_joy2_dpad_right_up() { imp.joy2 &= ~MN_INPUT_RIGHT; }
static void imp_joy2_a_down() { imp.joy2 |= MN_INPUT_A; }
static void imp_joy2_a_up() { imp.joy2 &= ~MN_INPUT_A; }
static void imp_joy2_b_down() { imp.joy2 |= MN_INPUT_B; }
static void imp_joy2_b_up() { imp.joy2 &= ~MN_INPUT_B; }
static void imp_joy2_a_turbo_down() { imp.joy2 |= MN_INPUT_TURBO_A; }
static void imp_joy2_a_turbo_up() { imp.joy2 &= ~MN_INPUT_TURBO_A; }
static void imp_joy2_b_turbo_down() { imp.joy2 |= MN_INPUT_TURBO_B; }
static void imp_joy2_b_turbo_up() { imp.joy2 &= ~MN_INPUT_TURBO_B; }

static const imp_action_data imp_actions[IMP_ACTION_COUNT] = {
  [IMP_ACTION_JOY1_A] = { "bind_joy1_a", imp_joy1_a_down, imp_joy1_a_up },
  [IMP_ACTION_JOY1_B] = { "bind_joy1_b", imp_joy1_b_down, imp_joy1_b_up },
  [IMP_ACTION_JOY1_A_TURBO] = { "bind_joy1_a_turbo", imp_joy1_a_turbo_down, imp_joy1_a_turbo_up },
  [IMP_ACTION_JOY1_B_TURBO] = { "bind_joy1_b_turbo", imp_joy1_b_turbo_down, imp_joy1_b_turbo_up },
  [IMP_ACTION_JOY1_START] = { "bind_joy1_start", imp_joy1_start_down, imp_joy1_start_up },
  [IMP_ACTION_JOY1_SELECT] = { "bind_joy1_select", imp_joy1_select_down, imp_joy1_select_up },
  [IMP_ACTION_JOY1_UP] = { "bind_joy1_up", imp_joy1_dpad_up_down, imp_joy1_dpad_up_up },
  [IMP_ACTION_JOY1_DOWN] = { "bind_joy1_down", imp_joy1_dpad_down_down, imp_joy1_dpad_down_up },
  [IMP_ACTION_JOY1_LEFT] = { "bind_joy1_left", imp_joy1_dpad_left_down, imp_joy1_dpad_left_up },
  [IMP_ACTION_JOY1_RIGHT] = { "bind_joy1_right", imp_joy1_dpad_right_down, imp_joy1_dpad_right_up },
  [IMP_ACTION_JOY2_A] = { "bind_joy2_a", imp_joy2_a_down, imp_joy2_a_up },
  [IMP_ACTION_JOY2_B] = { "bind_joy2_b", imp_joy2_b_down, imp_joy2_b_up },
  [IMP_ACTION_JOY2_A_TURBO] = { "bind_joy2_a_turbo", imp_joy2_a_turbo_down, imp_joy2_a_turbo_up },
  [IMP_ACTION_JOY2_B_TURBO] = { "bind_joy2_b_turbo", imp_joy2_b_turbo_down, imp_joy2_b_turbo_up },
  [IMP_ACTION_JOY2_START] = { "bind_joy2_start", imp_joy1_start_down, imp_joy1_start_up },
  [IMP_ACTION_JOY2_SELECT] = { "bind_joy2_select", imp_joy1_select_down, imp_joy1_select_up },
  [IMP_ACTION_JOY2_UP] = { "bind_joy2_up", imp_joy2_dpad_up_down, imp_joy2_dpad_up_up },
  [IMP_ACTION_JOY2_DOWN] = { "bind_joy2_down", imp_joy2_dpad_down_down, imp_joy2_dpad_down_up },
  [IMP_ACTION_JOY2_LEFT] = { "bind_joy2_left", imp_joy2_dpad_left_down, imp_joy2_dpad_left_up },
  [IMP_ACTION_JOY2_RIGHT] = { "bind_joy2_right", imp_joy2_dpad_right_down, imp_joy2_dpad_right_up },
  [IMP_ACTION_MENU] = { "bind_menu", imp_toggle_menu, nullptr },
  [IMP_ACTION_HELP] = { "bind_help", imp_toggle_help, nullptr },
  [IMP_ACTION_OPEN] = { "bind_open", imp_open_file, nullptr },
  [IMP_ACTION_REGION] = { "bind_region", imp_toggle_region, nullptr },
  [IMP_ACTION_RESET] = { "bind_reset", imp_reset, nullptr },
  [IMP_ACTION_SAVE] = { "bind_save", imp_save, nullptr },
  [IMP_ACTION_ASPECT] = { "bind_aspect", imp_toggle_aspect, nullptr },
  [IMP_ACTION_OVERSCAN] = { "bind_overscan", imp_toggle_overscan, nullptr },
  [IMP_ACTION_LOAD] = { "bind_load", imp_load, nullptr },
  [IMP_ACTION_QUIT] = { "bind_quit", imp_quit, nullptr },
  [IMP_ACTION_FULLSCREEN] = { "bind_fullscreen", imp_toggle_fullscreen, nullptr },
  [IMP_ACTION_SCREENSHOT] = { "bind_screenshot", imp_screenshot, nullptr },
  [IMP_ACTION_PAUSE] = { "bind_pause", imp_toggle_pause, nullptr },
  [IMP_ACTION_SHOW_FPS] = { "bind_show_fps", imp_toggle_fps, nullptr },
  [IMP_ACTION_MAX_FPS] = { "bind_max_fps", imp_toggle_fps_test, nullptr },
  [IMP_ACTION_FAST_FORWARD] = { "bind_fast_forward", imp_fast_forward_on, imp_fast_forward_off },
  [IMP_ACTION_SLOT0] = { "bind_slot0", imp_slot0, nullptr },
  [IMP_ACTION_SLOT1] = { "bind_slot1", imp_slot1, nullptr },
  [IMP_ACTION_SLOT2] = { "bind_slot2", imp_slot2, nullptr },
  [IMP_ACTION_SLOT3] = { "bind_slot3", imp_slot3, nullptr },
  [IMP_ACTION_SLOT4] = { "bind_slot4", imp_slot4, nullptr },
  [IMP_ACTION_SLOT5] = { "bind_slot5", imp_slot5, nullptr },
  [IMP_ACTION_SLOT6] = { "bind_slot6", imp_slot6, nullptr },
  [IMP_ACTION_SLOT7] = { "bind_slot7", imp_slot7, nullptr },
  [IMP_ACTION_SLOT8] = { "bind_slot8", imp_slot8, nullptr },
  [IMP_ACTION_SLOT9] = { "bind_slot9", imp_slot9, nullptr },
  [IMP_ACTION_SLOT_NEXT] = { "bind_slot_next", imp_slot_next, nullptr },
  [IMP_ACTION_SLOT_PREV] = { "bind_slot_prev", imp_slot_prev, nullptr },
  [IMP_ACTION_VOLUME_UP] = { "bind_volume_up", imp_volume_up, nullptr },
  [IMP_ACTION_VOLUME_DOWN] = { "bind_volume_down", imp_volume_down, nullptr },
  [IMP_ACTION_SCALE_UP] = { "bind_scale_up", imp_upscale, nullptr },
  [IMP_ACTION_SCALE_DOWN] = { "bind_scale_down", imp_downscale, nullptr },
};

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
    } else if (strcmp(key, "embed") == 0) {
      fscanf(f, " \"%[^\"]\"", imp.load_rom_path);
      if (imp.load_rom_path[0]) {
        SDL_PathInfo info;
        SDL_GetPathInfo(imp.load_rom_path, &info);
        if (info.type == SDL_PATHTYPE_FILE) {
          imp.embed_enabled = true;
        }
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

#ifdef __EMSCRIPTEN__
  imp_read_config("embed.txt");
#endif
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

static void imp_drop_file(const char* path)
{
  if (path && !imp.embed_enabled) {
    imp_copy_tmp_file(path, imp.load_rom_path);
    imp_rom_load(imp.load_rom_path);
    imp_rom_update();
  }
}

#define imp_draw_line(line, fmt, ...)                                                    \
  SDL_RenderDebugTextFormat(imp.renderer, imp.ui_offset.x, imp.ui_offset.y + 8 * (line), \
                            fmt __VA_OPT__(, ) __VA_ARGS__);

static bool imp_menu_pressed(uint mask) { return (imp.menu_input & mask) && !(imp.menu_input_prev & mask); }

static bool imp_draw_menu_line_raw(const char* label, uint line, int selection_id)
{
  bool selected = imp.menu_selection == selection_id;
  if (selected) {
    SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
  }
  imp_draw_line(line, "          %s %s", selected ? "*" : " ", label);
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  return selected;
}

static bool imp_draw_menu_line(const char* label, uint line, int selection_id)
{
  if (imp_draw_menu_line_raw(label, line, selection_id)) {
    if (imp_menu_pressed(MN_INPUT_A | MN_INPUT_START)) {
      return true;
    }
  }
  return false;
}

static uint imp_draw_menu_line_arrows(const char* label, uint line, int selection_id)
{
  if (imp_draw_menu_line_raw(label, line, selection_id)) {
    if (imp_menu_pressed(MN_INPUT_LEFT)) {
      return MN_INPUT_LEFT;
    }
    if (imp_menu_pressed(MN_INPUT_RIGHT)) {
      return MN_INPUT_RIGHT;
    }
  }
  return 0;
}

static void imp_draw_menu_prepare(int menu_size)
{
  imp.menu_input_prev = imp.menu_input;
  imp.menu_input = (imp.joy1 | imp.joy2);

  if (imp_menu_pressed(MN_INPUT_DOWN | MN_INPUT_SELECT)) {
    ++imp.menu_selection;
  }
  if (imp_menu_pressed(MN_INPUT_UP)) {
    --imp.menu_selection;
  }
  if (imp.menu_selection >= menu_size) {
    imp.menu_selection = 0;
  }
  if (imp.menu_selection < 0) {
    imp.menu_selection = menu_size - 1;
  }

  SDL_FRect fill = { imp.ui_offset.x, imp.ui_offset.y, 256, 240 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xE0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

static void imp_draw_menu()
{
  bool rom_loaded = imp.rom && !imp.rom->mapper_error;
  imp_draw_menu_prepare(rom_loaded ? 9 : 4);
  uint line = 2;
  uint menu = 0;
  line += 4;
  if (rom_loaded) {
    if (imp_draw_menu_line("Continue", line++, menu++) || imp_menu_pressed(MN_INPUT_B)) {
      imp_menu_state(IMP_MENU_OFF);
    }
    ++line;
    switch (imp_draw_menu_line_arrows(imp_save_slot_labels[imp.slot], line++, menu++)) {
      case MN_INPUT_RIGHT: imp_slot_next(); break;
      case MN_INPUT_LEFT: imp_slot_prev(); break;
    }
    if (imp_draw_menu_line("Save", line++, menu++)) {
      imp_save();
    }
    if (imp_draw_menu_line("Load", line++, menu++)) {
      if (imp_load_check()) {
        imp_menu_state(IMP_MENU_OFF);
      }
    }
    if (!imp.embed_enabled && imp_draw_menu_line("ROM info", line++, menu++)) {
      imp_menu_state(IMP_MENU_INFO);
    }
    ++line;
  }
  if (!imp.embed_enabled) {
    if (imp_draw_menu_line("Open ROM", line++, menu++)) {
      imp_open_file();
    }
    if (imp_draw_menu_line("Recent ROMs", line++, menu++)) {
      imp_menu_state(IMP_MENU_RECENT);
    }
    ++line;
  }
  if (imp_draw_menu_line("Options", line++, menu++)) {
    imp_menu_state(IMP_MENU_OPTIONS);
  }
  if (imp_draw_menu_line("Keymap", line++, menu++)) {
    imp_menu_state(IMP_MENU_HELP);
  }
  ++line;
  ++line;
  if (!imp.embed_enabled) {
    imp_draw_line(line++, " Drag-n-drop *.nes file to play ");
    ++line;
  }
  imp_draw_line(line++, "     Press F1 to show keymap    ");
  ++line;
  imp_draw_line(line++, "     Touch to show controls     ");

  if (!imp.embed_enabled) {
    imp_draw_line(27, "                MyaowNES v" MN_VERSION " ");
  }
}

static void imp_draw_start()
{
  imp_draw_menu_prepare(0);
  uint line = 10;
  imp_draw_line(line++, "    Press any button to start   ");
  line += 4;
  imp_draw_line(line++, "     Press F1 to show keymap    ");
  ++line;
  imp_draw_line(line++, "     Touch to show controls     ");
  if (imp_menu_pressed(-1)) {
    imp_menu_state(IMP_MENU_OFF);
  }
}

static void imp_draw_options()
{
  imp_draw_menu_prepare(12);
  uint line = 8;
  uint menu = 0;
  if (imp_draw_menu_line("Back", line++, menu++) || imp_menu_pressed(MN_INPUT_B)) {
    imp_menu_state(IMP_MENU_MAIN);
  }
  ++line;
  Uint32 flags = SDL_GetWindowFlags(imp.window);
  bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
  if (imp_draw_menu_line(imp_fullscreen_labels[is_fullscreen], line++, menu++)) {
    imp_toggle_fullscreen();
  }
  switch (imp_draw_menu_line_arrows(imp_volume_labels[imp.volume], line++, menu++)) {
    case MN_INPUT_LEFT: imp_volume_down(); break;
    case MN_INPUT_RIGHT: imp_volume_up(); break;
  }
  switch (imp_draw_menu_line_arrows(imp_scale_labels[imp.scale - 1], line++, menu++)) {
    case MN_INPUT_LEFT: imp_downscale(); break;
    case MN_INPUT_RIGHT: imp_upscale(); break;
  }
  if (imp_draw_menu_line(imp_aspect_labels[imp.auto_aspect], line++, menu++)) {
    imp_toggle_aspect();
  }
  if (imp_draw_menu_line(imp_overscan_labels[imp.overscan], line++, menu++)) {
    imp_toggle_overscan();
  }
  if (imp_draw_menu_line(imp_fps_labels[imp.fps], line++, menu++)) {
    imp_toggle_fps();
  }
  if (imp_draw_menu_line(imp_fps_test_labels[imp.fps_test], line++, menu++)) {
    imp_toggle_fps_test();
  }
  if (imp_draw_menu_line(imp_region_labels[imp.region], line++, menu++)) {
    imp_toggle_region();
  }
  switch (imp_draw_menu_line_arrows(imp_fast_forward_labels[imp.fast_forward_scale - 2], line++, menu++)) {
    case MN_INPUT_LEFT: imp_fast_forward_down(); break;
    case MN_INPUT_RIGHT: imp_fast_forward_up(); break;
  }
  imp.auto_save_period = (imp.auto_save_period / 30) * 30;
  if (imp.auto_save_period > 300) {
    imp.auto_save_period = 300;
  }
  switch (imp_draw_menu_line_arrows(imp_auto_save_period_labels[imp.auto_save_period / 30], line++, menu++)) {
    case MN_INPUT_LEFT: imp_auto_save_period_down(); break;
    case MN_INPUT_RIGHT: imp_auto_save_period_up(); break;
  }
  // todo: auto_save_period
  // todo: fast_forward_scale
  ++line;
  if (imp_draw_menu_line("Defaults", line++, menu++)) {
    imp_reset_config();
  }
}

static void imp_draw_info()
{
  imp_draw_menu_prepare(1);
  uint line = 8;
  uint menu = 0;
  if (imp_draw_menu_line("Back", line++, menu++) || imp_menu_pressed(MN_INPUT_B)) {
    imp_menu_state(IMP_MENU_MAIN);
  }
  ++line;
  imp_draw_line(line++, "          ROM %s", !imp.rom || imp.rom->mapper_error ? "ERROR!" : "loaded");
  ++line;
  imp_draw_line(line++, "          %s", imp.rom_info[0]);
  imp_draw_line(line++, "          %s", imp.rom_info[1]);
  imp_draw_line(line++, "          %s", imp.rom_info[2]);
  imp_draw_line(line++, "          %s", imp.rom_info[3]);
  imp_draw_line(line++, "          %s", imp.rom_info[4]);
  imp_draw_line(line++, "          %s", imp.rom_info[5]);
}

#define K(action) SDL_GetScancodeName(imp.scancode_by_action[action])

static void imp_draw_help()
{
  imp_draw_menu_prepare(1);
  uint line = 2;
  uint menu = 0;
  if (imp_draw_menu_line("Back", line++, menu++) || imp_menu_pressed(-1)) {
    imp_toggle_help();
  }
  ++line;
  imp_draw_line(line++, "           Joy1            Joy2 ");
  imp_draw_line(line++, " D-pad %2.2s%2.2s%2.2s%2.2s        %2.2s%2.2s%2.2s%2.2s ", K(IMP_ACTION_JOY1_UP),
                K(IMP_ACTION_JOY1_LEFT), K(IMP_ACTION_JOY1_DOWN), K(IMP_ACTION_JOY1_RIGHT), K(IMP_ACTION_JOY2_UP),
                K(IMP_ACTION_JOY2_LEFT), K(IMP_ACTION_JOY2_DOWN), K(IMP_ACTION_JOY2_RIGHT));
  imp_draw_line(line++, " B %12.12s  %14.14s ", K(IMP_ACTION_JOY1_B), K(IMP_ACTION_JOY2_B));
  imp_draw_line(line++, " A %12.12s  %14.14s ", K(IMP_ACTION_JOY1_A), K(IMP_ACTION_JOY2_A));
  imp_draw_line(line++, " B turbo %6.6s  %14.14s ", K(IMP_ACTION_JOY1_B_TURBO), K(IMP_ACTION_JOY2_B_TURBO));
  imp_draw_line(line++, " A turbo %6.6s  %14.14s ", K(IMP_ACTION_JOY1_A_TURBO), K(IMP_ACTION_JOY2_A_TURBO));
  imp_draw_line(line++, " Start %8.8s  %14.14s ", K(IMP_ACTION_JOY1_START), K(IMP_ACTION_JOY2_START));
  imp_draw_line(line++, " Select %7.7s  %14.14s ", K(IMP_ACTION_JOY1_SELECT), K(IMP_ACTION_JOY2_SELECT));
  imp_draw_line(line++, "                                ");
  imp_draw_line(line++, " Help %9.9s  Menu %9.9s ", K(IMP_ACTION_HELP), K(IMP_ACTION_MENU));
  if (!imp.embed_enabled) {
    imp_draw_line(line++, " Open %9.9s   ", K(IMP_ACTION_OPEN));
  }
  imp_draw_line(line++, " Region %7.7s  F-forward %4.4s ", K(IMP_ACTION_REGION), K(IMP_ACTION_FAST_FORWARD));
  imp_draw_line(line++, " Reset %8.8s  Volume %3.3s/%-3.3s ", K(IMP_ACTION_RESET), K(IMP_ACTION_VOLUME_DOWN),
                K(IMP_ACTION_VOLUME_UP));
  imp_draw_line(line++, " Save %9.9s  Slot   %3.3s-%-3.3s ", K(IMP_ACTION_SAVE), K(IMP_ACTION_SLOT0),
                K(IMP_ACTION_SLOT9));
  imp_draw_line(line++, " Aspect %7.7s  Pause %8.8s ", K(IMP_ACTION_ASPECT), K(IMP_ACTION_PAUSE));
  imp_draw_line(line++, " Overscan %5.5s  Show FPS %5.5s ", K(IMP_ACTION_OVERSCAN), K(IMP_ACTION_SHOW_FPS));
  imp_draw_line(line++, " Load %9.9s                 ", K(IMP_ACTION_LOAD));
#ifndef __EMSCRIPTEN__
  imp_draw_line(line++, " Quit %9.9s  Scale  %3.3s/%-3.3s ", K(IMP_ACTION_QUIT), K(IMP_ACTION_SCALE_DOWN),
                K(IMP_ACTION_SCALE_UP));
#endif
  imp_draw_line(line++, " Fullscreen %3.3s                 ", K(IMP_ACTION_FULLSCREEN));
#ifndef __EMSCRIPTEN__
  imp_draw_line(line++, " Screenshot %3.3s  Max FPS %6.6s ", K(IMP_ACTION_SCREENSHOT), K(IMP_ACTION_MAX_FPS));
#endif
  ++line;
  imp_draw_line(line++, "     Controller");
  imp_draw_line(line++, " F-forward   LB  Menu        RB ");
  imp_draw_line(line++, " Save        LS  Load        RS ");
}

#undef K

static void imp_draw_recent()
{
  uint count = 0;
  for (; count < IMP_RECENT_COUNT; ++count) {
    if (imp.recent_files[count][0] == 0) {
      break;
    }
  }
  imp_draw_menu_prepare(count + 1);
  uint line = 4;
  uint menu = 0;
  if (imp_draw_menu_line("Back", line++, menu++) || imp_menu_pressed(MN_INPUT_B)) {
    imp_menu_state(IMP_MENU_MAIN);
  }
  ++line;
  for (uint i = 0; i < count; ++i) {
    if (imp_draw_menu_line(imp_rom_name(imp.recent_files[i]), line++, menu++)) {
      imp_rom_load(imp.recent_files[i]);
      imp_rom_update();
    }
  }
}

static void imp_draw_touch_button(const char* label, uint x, uint y, uint r)
{
  SDL_FRect rect = { x - r, y - r, 2 * r, 2 * r };
  SDL_RenderRect(imp.renderer, &rect);
  SDL_RenderDebugText(imp.renderer, x - 4, y - 4, label);
}

static void imp_draw_touch()
{
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0x80, 0x80, 0x80);

  imp_draw_touch_button("M", imp.touch_menu_x, imp.touch_menu_y, imp.touch_button_radius);
  imp_draw_touch_button("F", imp.touch_ff_x, imp.touch_ff_y, imp.touch_button_radius);
  imp_draw_touch_button("St", imp.touch_start_x, imp.touch_start_y, imp.touch_button_radius);
  imp_draw_touch_button("Se", imp.touch_select_x, imp.touch_select_y, imp.touch_button_radius);
  imp_draw_touch_button("A", imp.touch_a_x, imp.touch_a_y, imp.touch_button_ab_radius);
  imp_draw_touch_button("B", imp.touch_b_x, imp.touch_b_y, imp.touch_button_ab_radius);
  imp_draw_touch_button("+", imp.touch_dpad_x, imp.touch_dpad_y, imp.touch_dpad_radius);
  imp_draw_touch_button("", imp.touch_dpad_x, imp.touch_dpad_y, imp.touch_dpad_threshold);
}

static bool imp_touch_hit(float ex, float ey, int x, int y, int r)
{
  int dx = ex - x;
  int dy = ey - y;
  return (dx * dx + dy * dy) < (r * r);
}

static void imp_touch_cancel(SDL_FingerID fingerID)
{
  if (imp.touch_button[IMP_TOUCH_A] == fingerID) {
    imp.touch_button[IMP_TOUCH_A] = 0;
    imp_joy1_a_up();
  }
  if (imp.touch_button[IMP_TOUCH_B] == fingerID) {
    imp.touch_button[IMP_TOUCH_B] = 0;
    imp_joy1_b_up();
  }
  if (imp.touch_button[IMP_TOUCH_START] == fingerID) {
    imp.touch_button[IMP_TOUCH_START] = 0;
    imp_joy1_start_up();
  }
  if (imp.touch_button[IMP_TOUCH_SELECT] == fingerID) {
    imp.touch_button[IMP_TOUCH_SELECT] = 0;
    imp_joy1_select_up();
  }
  if (imp.touch_button[IMP_TOUCH_FF] == fingerID) {
    imp.touch_button[IMP_TOUCH_FF] = 0;
    imp_fast_forward_off();
  }
  if (imp.touch_button[IMP_TOUCH_UP] == fingerID) {
    imp.touch_button[IMP_TOUCH_UP] = 0;
    imp_joy1_dpad_up_up();
  }
  if (imp.touch_button[IMP_TOUCH_DOWN] == fingerID) {
    imp.touch_button[IMP_TOUCH_DOWN] = 0;
    imp_joy1_dpad_down_up();
  }
  if (imp.touch_button[IMP_TOUCH_LEFT] == fingerID) {
    imp.touch_button[IMP_TOUCH_LEFT] = 0;
    imp_joy1_dpad_left_up();
  }
  if (imp.touch_button[IMP_TOUCH_RIGHT] == fingerID) {
    imp.touch_button[IMP_TOUCH_RIGHT] = 0;
    imp_joy1_dpad_right_up();
  }
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

SDL_AppResult SDL_AppInit(void**, int argc, char* argv[])
{
  SDL_Log("MyaowNES v%s", MN_VERSION);
  SDL_memset(&imp, 0, sizeof(imp));

  if (SDL_strlen(imp.save_dir) == 0) {
    char* pref_path = SDL_GetPrefPath(nullptr, "MyaowNES");
    SDL_strlcpy(imp.save_dir, pref_path, IMP_PATH_SIZE);
    SDL_free(pref_path);
  }

  SDL_Log("Save path: %s", imp.save_dir);

  imp_ensure_dir("Saves");
  imp_ensure_dir("Screenshots");
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
  imp_ensure_dir("Roms");
#endif

  SDL_strlcat(imp.config_path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(imp.config_path, "config.txt", IMP_PATH_SIZE);

  SDL_strlcat(imp.history_path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(imp.history_path, "history.txt", IMP_PATH_SIZE);

  imp_load_config();
  imp_load_history();
  imp_volume_update();
  mn_region_set(imp.region);

  if (imp.embed_enabled) {
    imp_rom_load(imp.load_rom_path);
  }

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

  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  imp.perf_freq = SDL_GetPerformanceFrequency();

  uint width;
  uint height;
  mn_video_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);

  const char* title = imp.embed_enabled ? nullptr : "MyaowNES";
  if (!SDL_CreateWindowAndRenderer(title, width, height, SDL_WINDOW_RESIZABLE, &imp.window, &imp.renderer)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  int vsync;
  SDL_GetRenderVSync(imp.renderer, &vsync);
  SDL_Log("Video: %s %s %s", SDL_GetRendererName(imp.renderer), vsync ? "vsync:on" : "vsync:off",
          SDL_ScreenSaverEnabled() ? "screensaver:on" : "screensaver:off");

  imp.tex_out = SDL_CreateTexture(imp.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!imp.tex_out) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(imp.tex_out, SDL_SCALEMODE_PIXELART);

  imp.audioDeviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  SDL_AudioSpec spec;
  int samples;
  SDL_GetAudioDeviceFormat(imp.audioDeviceId, &spec, &samples);
  SDL_AudioSpec stream_spec = { SDL_AUDIO_S16, 1, spec.freq };
  imp.stream = SDL_OpenAudioDeviceStream(imp.audioDeviceId, &stream_spec, nullptr, nullptr);
  SDL_ResumeAudioStreamDevice(imp.stream);
  SDL_Log("Audio: %s %d %s%d%s %dch %d", SDL_GetCurrentAudioDriver(), spec.freq,
          SDL_AUDIO_ISFLOAT(spec.format)      ? "F"
          : SDL_AUDIO_ISUNSIGNED(spec.format) ? "U"
                                              : "S",
          SDL_AUDIO_BITSIZE(spec.format), SDL_AUDIO_ISBIGENDIAN(spec.format) ? "BE" : "LE", spec.channels, samples);
  imp.audio_buffer_size = samples * sizeof(i16);
  mn_audio_freq(spec.freq);
  imp_volume_update();

  bool inited = imp_init();
  imp_rom_update();

  if (imp.embed_enabled && imp.start_screen && imp_audio_suspended()) {
    imp_menu_state(IMP_MENU_START);
  }

  return inited ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: imp_quit(); break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED: imp_ondisplaychange(); break;
    case SDL_EVENT_DROP_FILE: imp_drop_file(event->drop.data); break;
    // case SDL_EVENT_DISPLAY_ORIENTATION:
    //   imp_set_fullscreen(event->display.data1 == SDL_ORIENTATION_LANDSCAPE
    //                      || event->display.data1 == SDL_ORIENTATION_LANDSCAPE_FLIPPED);
    // break;
    case SDL_EVENT_KEY_DOWN: {
      imp.touch_active = false;
      void (*onkeydown)() = imp_actions[imp.action_by_scancode[event->key.scancode]].onkeydown;
      if (onkeydown) {
        onkeydown();
      } else {
        imp_popup("Press F1 to show keymap", 2);
      }
      break;
    }
    case SDL_EVENT_KEY_UP: {
      void (*onkeyup)() = imp_actions[imp.action_by_scancode[event->key.scancode]].onkeyup;
      if (onkeyup) {
        onkeyup();
      }
      break;
    }
    case SDL_EVENT_GAMEPAD_ADDED:
      if (!imp.gamepad1) {
        imp.gamepad1 = SDL_OpenGamepad(event->gdevice.which);
        imp.gamepad_id1 = event->gdevice.which;
      } else if (!imp.gamepad2) {
        imp.gamepad2 = SDL_OpenGamepad(event->gdevice.which);
        imp.gamepad_id2 = event->gdevice.which;
      }
      break;
    case SDL_EVENT_GAMEPAD_REMOVED:
      if (event->gdevice.which == imp.gamepad_id1) {
        SDL_CloseGamepad(imp.gamepad1);
        imp.gamepad1 = nullptr;
        imp.gamepad_id1 = 0;
      } else if (event->gdevice.which == imp.gamepad_id2) {
        SDL_CloseGamepad(imp.gamepad2);
        imp.gamepad2 = nullptr;
        imp.gamepad_id2 = 0;
      }
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      imp.touch_active = false;
      switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: imp_fast_forward_on(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: imp_toggle_menu(); break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: imp_save(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: imp_load(); break;
      }
      if (event->gbutton.which == imp.gamepad_id1) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: imp_joy1_dpad_up_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: imp_joy1_dpad_left_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: imp_joy1_dpad_down_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: imp_joy1_dpad_right_down(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: imp_joy1_b_down(); break;
          case SDL_GAMEPAD_BUTTON_EAST: imp_joy1_a_down(); break;
          case SDL_GAMEPAD_BUTTON_WEST: imp_joy1_b_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: imp_joy1_a_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_BACK: imp_joy1_select_down(); break;
          case SDL_GAMEPAD_BUTTON_START: imp_joy1_start_down(); break;
        }
      } else if (event->gbutton.which == imp.gamepad_id2) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: imp_joy2_dpad_up_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: imp_joy2_dpad_left_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: imp_joy2_dpad_down_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: imp_joy2_dpad_right_down(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: imp_joy2_b_down(); break;
          case SDL_GAMEPAD_BUTTON_EAST: imp_joy2_a_down(); break;
          case SDL_GAMEPAD_BUTTON_WEST: imp_joy2_b_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: imp_joy2_a_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_BACK: imp_joy1_select_down(); break;
          case SDL_GAMEPAD_BUTTON_START: imp_joy1_start_down(); break;
        }
      }
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: imp_fast_forward_off(); break;
      }
      if (event->gbutton.which == imp.gamepad_id1) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: imp_joy1_dpad_up_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: imp_joy1_dpad_left_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: imp_joy1_dpad_down_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: imp_joy1_dpad_right_up(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: imp_joy1_b_up(); break;
          case SDL_GAMEPAD_BUTTON_EAST: imp_joy1_a_up(); break;
          case SDL_GAMEPAD_BUTTON_WEST: imp_joy1_b_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: imp_joy1_a_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_BACK: imp_joy1_select_up(); break;
          case SDL_GAMEPAD_BUTTON_START: imp_joy1_start_up(); break;
        }
      } else if (event->gbutton.which == imp.gamepad_id2) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: imp_joy2_dpad_up_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: imp_joy2_dpad_left_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: imp_joy2_dpad_down_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: imp_joy2_dpad_right_up(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: imp_joy2_b_up(); break;
          case SDL_GAMEPAD_BUTTON_EAST: imp_joy2_a_up(); break;
          case SDL_GAMEPAD_BUTTON_WEST: imp_joy2_b_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: imp_joy2_a_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_BACK: imp_joy1_select_up(); break;
          case SDL_GAMEPAD_BUTTON_START: imp_joy1_start_up(); break;
        }
      }
      break;
#ifndef NDEBUG
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
#endif
    case SDL_EVENT_FINGER_DOWN: {
      if (!imp.touch_active) {
        break;
      }
      float ex = 0;
      float ey = 0;
      SDL_FingerID id = 0;
      if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        ex = event->button.x;
        ey = event->button.y;
        id = SDL_TOUCH_MOUSEID;
      } else {
        ex = event->tfinger.x * imp.width;
        ey = event->tfinger.y * imp.height;
        id = event->tfinger.fingerID;
      }
      if (imp_touch_hit(ex, ey, imp.touch_a_x, imp.touch_a_y, imp.touch_button_ab_radius)) {
        imp.touch_button[IMP_TOUCH_A] = id;
        imp_joy1_a_down();
      } else if (imp_touch_hit(ex, ey, imp.touch_b_x, imp.touch_b_y, imp.touch_button_ab_radius)) {
        imp.touch_button[IMP_TOUCH_B] = id;
        imp_joy1_b_down();
      } else if (imp_touch_hit(ex, ey, imp.touch_start_x, imp.touch_start_y, imp.touch_button_radius)) {
        imp.touch_button[IMP_TOUCH_START] = id;
        imp_joy1_start_down();
      } else if (imp_touch_hit(ex, ey, imp.touch_select_x, imp.touch_select_y, imp.touch_button_radius)) {
        imp.touch_button[IMP_TOUCH_SELECT] = id;
        imp_joy1_select_down();
      } else if (imp_touch_hit(ex, ey, imp.touch_menu_x, imp.touch_menu_y, imp.touch_button_radius)) {
        imp_toggle_menu();
      } else if (imp_touch_hit(ex, ey, imp.touch_ff_x, imp.touch_ff_y, imp.touch_button_radius)) {
        imp.touch_button[IMP_TOUCH_FF] = id;
        imp_fast_forward_on();
      } else {
        int dx = ex - imp.touch_dpad_x;
        int dy = ey - imp.touch_dpad_y;
        int d2 = dx * dx + dy * dy;
        int r2 = imp.touch_dpad_radius * imp.touch_dpad_radius;
        int t2 = imp.touch_dpad_threshold * imp.touch_dpad_threshold;
        if (d2 < r2 && d2 > t2) {
          float a = SDL_atan2f(dy, dx);
          if (a >= -0.875f * SDL_PI_F && a < -0.125f * SDL_PI_F) {
            imp.touch_button[IMP_TOUCH_UP] = id;
            imp_joy1_dpad_up_down();
          }
          if (a >= 0.125f * SDL_PI_F && a < 0.875f * SDL_PI_F) {
            imp.touch_button[IMP_TOUCH_DOWN] = id;
            imp_joy1_dpad_down_down();
          }
          if (a < -0.625f * SDL_PI_F || a >= 0.625f * SDL_PI_F) {
            imp.touch_button[IMP_TOUCH_LEFT] = id;
            imp_joy1_dpad_left_down();
          }
          if (a >= -0.375f * SDL_PI_F && a < 0.375f * SDL_PI_F) {
            imp.touch_button[IMP_TOUCH_RIGHT] = id;
            imp_joy1_dpad_right_down();
          }
        }
      }
      break;
    }
#ifndef NDEBUG
    case SDL_EVENT_MOUSE_BUTTON_UP:
#endif
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
      if (!imp.touch_active) {
        imp.touch_active = true;
        break;
      }
      imp_touch_cancel(event->type == SDL_EVENT_MOUSE_BUTTON_UP ? SDL_TOUCH_MOUSEID : event->tfinger.fingerID);
      break;
#ifndef NDEBUG
    case SDL_EVENT_MOUSE_MOTION:
#endif
    case SDL_EVENT_FINGER_MOTION: {
      if (!imp.touch_active) {
        break;
      }
      float ex = 0;
      float ey = 0;
      SDL_FingerID id = 0;
      if (event->type == SDL_EVENT_MOUSE_MOTION) {
        if (event->motion.state != 1) {
          break;
        }
        ex = event->motion.x;
        ey = event->motion.y;
        id = SDL_TOUCH_MOUSEID;
      } else {
        ex = event->tfinger.x * imp.width;
        ey = event->tfinger.y * imp.height;
        id = event->tfinger.fingerID;
      }
      if (imp_touch_hit(ex, ey, imp.touch_a_x, imp.touch_a_y, imp.touch_button_ab_radius)) {
        if (imp.touch_button[IMP_TOUCH_A] != id) {
          imp.touch_button[IMP_TOUCH_A] = id;
          imp_joy1_a_down();
        }
      } else if (imp.touch_button[IMP_TOUCH_A] == id) {
        imp.touch_button[IMP_TOUCH_A] = 0;
        imp_joy1_a_up();
      }
      if (imp_touch_hit(ex, ey, imp.touch_b_x, imp.touch_b_y, imp.touch_button_ab_radius)) {
        if (imp.touch_button[IMP_TOUCH_B] != id) {
          imp.touch_button[IMP_TOUCH_B] = id;
          imp_joy1_b_down();
        }
      } else if (imp.touch_button[IMP_TOUCH_B] == id) {
        imp.touch_button[IMP_TOUCH_B] = 0;
        imp_joy1_b_up();
      }
      {
        int dx = ex - imp.touch_dpad_x;
        int dy = ey - imp.touch_dpad_y;
        int d2 = dx * dx + dy * dy;
        int r2 = imp.touch_dpad_radius * imp.touch_dpad_radius;
        int t2 = imp.touch_dpad_threshold * imp.touch_dpad_threshold;
        bool hit = d2 < r2 && d2 > t2;
        float a = SDL_atan2f(dy, dx);
        if (hit && a >= -0.875f * SDL_PI_F && a < -0.125f * SDL_PI_F) {
          if (imp.touch_button[IMP_TOUCH_UP] != id) {
            imp.touch_button[IMP_TOUCH_UP] = id;
            imp_joy1_dpad_up_down();
          }
        } else if (imp.touch_button[IMP_TOUCH_UP] == id) {
          imp.touch_button[IMP_TOUCH_UP] = 0;
          imp_joy1_dpad_up_up();
        }
        if (hit && a >= 0.125f * SDL_PI_F && a < 0.875f * SDL_PI_F) {
          if (imp.touch_button[IMP_TOUCH_DOWN] != id) {
            imp.touch_button[IMP_TOUCH_DOWN] = id;
            imp_joy1_dpad_down_down();
          }
        } else if (imp.touch_button[IMP_TOUCH_DOWN] == id) {
          imp.touch_button[IMP_TOUCH_DOWN] = 0;
          imp_joy1_dpad_down_up();
        }
        if (hit && (a < -0.625f * SDL_PI_F || a >= 0.625f * SDL_PI_F)) {
          if (imp.touch_button[IMP_TOUCH_LEFT] != id) {
            imp.touch_button[IMP_TOUCH_LEFT] = id;
            imp_joy1_dpad_left_down();
          }
        } else if (imp.touch_button[IMP_TOUCH_LEFT] == id) {
          imp.touch_button[IMP_TOUCH_LEFT] = 0;
          imp_joy1_dpad_left_up();
        }
        if (hit && a >= -0.375f * SDL_PI_F && a < 0.375f * SDL_PI_F) {
          if (imp.touch_button[IMP_TOUCH_RIGHT] != id) {
            imp.touch_button[IMP_TOUCH_RIGHT] = id;
            imp_joy1_dpad_right_down();
          }
        } else if (imp.touch_button[IMP_TOUCH_RIGHT] == id) {
          imp.touch_button[IMP_TOUCH_RIGHT] = 0;
          imp_joy1_dpad_right_up();
        }
      }
      break;
    }
  }
  return imp.quit ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
  if (imp.load_rom_dialog_wait) {
    if (SDL_CompareAndSwapAtomicInt(&imp.load_rom_dialog_done, 1, 0)) {
      imp.load_rom_dialog_wait = false;
      if (imp.load_rom_path[0]) {
        imp_rom_load(imp.load_rom_path);
        imp_rom_update();
      }
    }
  }

  Uint64 time = SDL_GetPerformanceCounter();
  if (imp.last_time == 0) {
    imp.last_time = time;
  }
  imp.real_frame_time = time - imp.last_time;
  imp.last_time = time;

  Uint64 game_frame_time = imp.real_frame_time;
  if (imp.pause) {
    game_frame_time = 0;
  } else if (imp.fast_forward) {
    game_frame_time *= imp.fast_forward_scale;
    imp_popup(">>", 0.5);
  } else if (imp.fps_test) {
    game_frame_time = imp.target_frame_time;
  }
  imp.curr_time += game_frame_time;

  uint max_time = imp.target_frame_time * ((imp.fast_forward ? imp.fast_forward_scale : 1) + 1);
  if (imp.curr_time > max_time) {
    imp.curr_time = max_time;
  }

  uint queued_size = SDL_GetAudioStreamQueued(imp.stream);

  while (!imp.pause && (imp.curr_time >= imp.target_frame_time || queued_size < imp.audio_buffer_size)) {
    if (imp.fps) {
      ++imp.fps_count;
    }

    if (imp.tas) {
      mn_tas_frame(imp.tas);
    } else {
      mn_frame(imp.joy1, imp.joy2);
    }
    imp.curr_time = imp.curr_time > imp.target_frame_time ? imp.curr_time - imp.target_frame_time : 0;

    uint audio_size = mn_audio_size();
    uint target_size = imp.audio_buffer_size > audio_size ? imp.audio_buffer_size : audio_size;
    if (queued_size < 3 * target_size) {
      SDL_PutAudioStreamData(imp.stream, mn_audio_data(), audio_size);
      queued_size += audio_size;
    }
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
    if (imp.rom) {
      mn_rom_save(imp.sram_path);
    }
  }
  imp_save_config();

  SDL_SetRenderScale(imp.renderer, 1, 1);
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xFF);
  SDL_RenderClear(imp.renderer);

  SDL_UpdateTexture(imp.tex_out, nullptr, mn_video_data(), 256 * 4);
  SDL_RenderTexture(imp.renderer, imp.tex_out, nullptr, &imp.rect);

  SDL_SetRenderScale(imp.renderer, imp.ui_offset.w, imp.ui_offset.h);
  switch (imp.menu_state) {
    case IMP_MENU_MAIN: imp_draw_menu(); break;
    case IMP_MENU_OPTIONS: imp_draw_options(); break;
    case IMP_MENU_INFO: imp_draw_info(); break;
    case IMP_MENU_HELP: imp_draw_help(); break;
    case IMP_MENU_RECENT: imp_draw_recent(); break;
    case IMP_MENU_START: imp_draw_start(); break;
  }
  if (imp.popup_time > 0) {
    imp_draw_popup();
  }
  if (imp.fps) {
    SDL_SetRenderDrawColor(imp.renderer, 0x80, 0, 0x80, 0xFF);
    SDL_RenderDebugTextFormat(imp.renderer, imp.ui_offset.x + 256 - 4 * 8, imp.ui_offset.y + 21, "%4d",
                              (int)imp.fps_value);
    SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
    SDL_RenderDebugTextFormat(imp.renderer, imp.ui_offset.x + 256 - 4 * 8 - 1, imp.ui_offset.y + 20, "%4d",
                              (int)imp.fps_value);
  }
  if (imp.touch_active) {
    SDL_SetRenderScale(imp.renderer, 1, 1);
    imp_draw_touch();
  }

  SDL_RenderPresent(imp.renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult)
{
  imp_rom_unload();
  imp_save_config();
  if (imp.gamepad1) {
    SDL_CloseGamepad(imp.gamepad1);
  }
  if (imp.gamepad2) {
    SDL_CloseGamepad(imp.gamepad2);
  }
  SDL_DestroyAudioStream(imp.stream);
  SDL_CloseAudioDevice(imp.audioDeviceId);
  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

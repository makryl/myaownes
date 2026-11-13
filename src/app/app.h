#pragma once

#include "myaownes.h"
#include <SDL3/SDL.h>

#define MN_VERSION "000000"

enum
{
  APP_PATH_SIZE = 4096,
  APP_RECENT_COUNT = 20,
};

enum
{
  APP_MENU_OFF = 0,
  APP_MENU_MAIN,
  APP_MENU_OPTIONS,
  APP_MENU_INFO,
  APP_MENU_HELP,
  APP_MENU_RECENT,
  APP_MENU_START,
};

enum
{
  APP_TOUCH_A = 0,
  APP_TOUCH_B,
  APP_TOUCH_START,
  APP_TOUCH_SELECT,
  APP_TOUCH_UP,
  APP_TOUCH_DOWN,
  APP_TOUCH_LEFT,
  APP_TOUCH_RIGHT,
  APP_TOUCH_MENU,
  APP_TOUCH_FF,
  APP_TOUCH_DPAD,
  APP_TOUCH_COUNT,
};

typedef enum
{
  APP_ACTION_NONE = 0,
  APP_ACTION_JOY1_A,
  APP_ACTION_JOY1_B,
  APP_ACTION_JOY1_A_TURBO,
  APP_ACTION_JOY1_B_TURBO,
  APP_ACTION_JOY1_START,
  APP_ACTION_JOY1_SELECT,
  APP_ACTION_JOY1_UP,
  APP_ACTION_JOY1_DOWN,
  APP_ACTION_JOY1_LEFT,
  APP_ACTION_JOY1_RIGHT,
  APP_ACTION_JOY2_A,
  APP_ACTION_JOY2_B,
  APP_ACTION_JOY2_A_TURBO,
  APP_ACTION_JOY2_B_TURBO,
  APP_ACTION_JOY2_START,
  APP_ACTION_JOY2_SELECT,
  APP_ACTION_JOY2_UP,
  APP_ACTION_JOY2_DOWN,
  APP_ACTION_JOY2_LEFT,
  APP_ACTION_JOY2_RIGHT,
  APP_ACTION_MENU,
  APP_ACTION_HELP,
  APP_ACTION_OPEN,
  APP_ACTION_REGION,
  APP_ACTION_RESET,
  APP_ACTION_SAVE,
  APP_ACTION_ASPECT,
  APP_ACTION_OVERSCAN,
  APP_ACTION_LOAD,
  APP_ACTION_QUIT,
  APP_ACTION_FULLSCREEN,
  APP_ACTION_SCREENSHOT,
  APP_ACTION_PAUSE,
  APP_ACTION_SHOW_FPS,
  APP_ACTION_MAX_FPS,
  APP_ACTION_FAST_FORWARD,
  APP_ACTION_REWIND,
  APP_ACTION_SLOT0,
  APP_ACTION_SLOT1,
  APP_ACTION_SLOT2,
  APP_ACTION_SLOT3,
  APP_ACTION_SLOT4,
  APP_ACTION_SLOT5,
  APP_ACTION_SLOT6,
  APP_ACTION_SLOT7,
  APP_ACTION_SLOT8,
  APP_ACTION_SLOT9,
  APP_ACTION_SLOT_NEXT,
  APP_ACTION_SLOT_PREV,
  APP_ACTION_VOLUME_UP,
  APP_ACTION_VOLUME_DOWN,
  APP_ACTION_SCALE_UP,
  APP_ACTION_SCALE_DOWN,
  APP_ACTION_COUNT,
} app_action;

typedef struct
{
  const char* label;
  void (*onkeydown)();
  void (*onkeyup)();
} app_action_data;

typedef struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* tex_out;
  SDL_Texture* tex_font;
  SDL_Texture* tex_buttons;
  SDL_Gamepad* gamepad1;
  SDL_Gamepad* gamepad2;
  SDL_AudioDeviceID audioDeviceId;
  SDL_AudioStream* stream;
  SDL_JoystickID gamepad_id1;
  SDL_JoystickID gamepad_id2;
  SDL_FRect rect;
  SDL_FRect ui_offset;
  const char* popup_text;
  void* rewind_buffer;
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
  u64 touch_button[APP_TOUCH_COUNT];
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
  uint rewind_seconds;
  uint rewind_start;
  uint rewind_curr;
  uint rewind_scale;
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
  bool rewind;
  mn_rom rom;
  mn_tas tas;
  char rom_info[6][15];
  char load_rom_path[APP_PATH_SIZE];
  char rom_path[APP_PATH_SIZE];
  char sram_path[APP_PATH_SIZE];
  char save_dir[APP_PATH_SIZE];
  char default_config_path[APP_PATH_SIZE];
  char config_path[APP_PATH_SIZE];
  char history_path[APP_PATH_SIZE];
  char palette_path[APP_PATH_SIZE];
  char atlas_path[APP_PATH_SIZE];
  char recent_files[APP_RECENT_COUNT][APP_PATH_SIZE];
  SDL_Scancode scancode_by_action[APP_ACTION_COUNT];
  app_action action_by_scancode[SDL_SCANCODE_COUNT];
} app;

extern const app_action_data app_actions[APP_ACTION_COUNT];
extern app imp;

static const char* const app_save_slot_labels[10] = { "Slot 0", "Slot 1", "Slot 2", "Slot 3", "Slot 4",
                                                      "Slot 5", "Slot 6", "Slot 7", "Slot 8", "Slot 9" };
static const char* const app_scale_labels[8] = { "Scale 1", "Scale 2", "Scale 3", "Scale 4",
                                                 "Scale 5", "Scale 6", "Scale 7", "Scale 8" };
static const char* const app_volume_labels[11] = { "Volume OFF", "Volume 1", "Volume 2", "Volume 3",
                                                   "Volume 4",   "Volume 5", "Volume 6", "Volume 7",
                                                   "Volume 8",   "Volume 9", "Volume 10" };
static const char* const app_region_labels[4] = { "Region NTSC", "Region PAL", "Region Auto", "Region Dendy" };
static const char* const app_aspect_labels[2] = { "Aspect OFF", "Aspect ON" };
static const char* const app_overscan_labels[2] = { "Overscan OFF", "Overscan ON" };
static const char* const app_fps_labels[2] = { "Show FPS OFF", "Show FPS ON" };
static const char* const app_fps_test_labels[2] = { "Max FPS OFF", "Max FPS ON" };
static const char* const app_fullscreen_labels[2] = { "Fullscreen OFF", "Fullscreen ON" };
static const char* const app_fast_forward_labels[4] = { "Fast forward x2", "Fast forward x3", "Fast forward x4",
                                                        "Fast forward x5" };
static const char* const app_rewind_scale_labels[5] = { "Rewind speed x1", "Rewind speed x2", "Rewind speed x3",
                                                        "Rewind speed x4", "Rewind speed x5" };
static const char* const app_rewind_seconds_labels[7] = { "Rewind OFF",        "Rewind seconds 10", "Rewind seconds 20",
                                                          "Rewind seconds 30", "Rewind seconds 40", "Rewind seconds 50",
                                                          "Rewind seconds 60" };
static const char* const app_auto_save_period_labels[11] = {
  "Auto-save OFF",     "Auto-save 30 sec",  "Auto-save 60 sec",  "Auto-save 90 sec",
  "Auto-save 120 sec", "Auto-save 150 sec", "Auto-save 180 sec", "Auto-save 210 sec",
  "Auto-save 240 sec", "Auto-save 270 sec", "Auto-save 300 sec"
};

void app_ondisplaychange();
void app_try_resize();
void app_volume_update();
const char* app_rom_name(const char* path);
void app_save_path(char* dst, const char* subdir, const char* ext);
void app_tas_unload();
void app_tas_load(const char* path);
void app_rom_unload();
void app_rom_load(const char* path);
void app_rom_update();
void app_save_config();
void app_load_history();
void app_load_config();
void app_reset_config();
void app_rewind_init();
void app_rewind_save();
void app_rewind_load();

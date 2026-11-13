#pragma once

#include "myaownes.h"

void app_popup(const char* text, float time);
void app_reload();
void app_reset();
void app_menu_state(uint state);
void app_slot_next();
void app_slot_prev();
bool app_load_check();
void app_load();
void app_save();
void app_open_file();
void app_toggle_fullscreen();
void app_volume_down();
void app_volume_up();
void app_downscale();
void app_upscale();
void app_toggle_aspect();
void app_toggle_overscan();
void app_toggle_fps();
void app_toggle_fps_test();
void app_toggle_region();
void app_toggle_help();
void app_fast_forward_down();
void app_fast_forward_up();
void app_rewind_scale_down();
void app_rewind_scale_up();
void app_rewind_seconds_down();
void app_rewind_seconds_up();
void app_auto_save_period_down();
void app_auto_save_period_up();

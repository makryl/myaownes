#pragma once

#include "app.h"
#include "actions.h"

static void imp_draw_popup()
{
  imp.popup_time -= imp.popup_time > imp.real_frame_time ? imp.real_frame_time : imp.popup_time;
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 2, imp.ui_offset.y + 21, imp.popup_text);
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 1, imp.ui_offset.y + 20, imp.popup_text);
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

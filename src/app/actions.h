#pragma once

#include "app.h"

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

static void imp_start_off()
{
  if (imp.menu_state == IMP_MENU_START) {
    imp_menu_state(IMP_MENU_OFF);
  }
}

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

static void imp_drop_file(const char* path)
{
  if (path && !imp.embed_enabled) {
    imp_copy_tmp_file(path, imp.load_rom_path);
    imp_rom_load(imp.load_rom_path);
    imp_rom_update();
  }
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

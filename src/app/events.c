#include "app.h"

void app_popup(const char* text, float time)
{
  imp.popup_text = text;
  imp.popup_time = imp.perf_freq * time;
}

void app_power()
{
  mn_power();
  app_popup("Power", 2);
}

void app_reset()
{
  mn_reset();
  app_popup("Reset", 2);
}

void app_save()
{
  char qs_path[APP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  app_save_path(qs_path, "Saves", ext);
  if (mn_save(qs_path)) {
    app_popup("Saved", 2);
  }
}

bool app_load_check()
{
  char qs_path[APP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  app_save_path(qs_path, "Saves", ext);
  bool result = mn_load(qs_path);
  if (result) {
    app_popup("Loaded", 2);
  }
  return result;
}

void app_load() { app_load_check(); }

void app_menu_state(uint state)
{
  if (state == APP_MENU_OFF && !imp.rom) {
    state = APP_MENU_MAIN;
  }
  if (state == imp.menu_state) {
    return;
  }
  if (imp.menu_state != APP_MENU_HELP) {
    imp.menu_state_prev = imp.menu_state;
  }
  imp.menu_state = state;
  imp.menu_selection = 0;
  imp.pause = state != APP_MENU_OFF;
}

static void app_toggle_menu() { app_menu_state(imp.menu_state == APP_MENU_MAIN ? APP_MENU_OFF : APP_MENU_MAIN); }
void app_toggle_help() { app_menu_state(imp.menu_state == APP_MENU_HELP ? imp.menu_state_prev : APP_MENU_HELP); }

static void app_start_off()
{
  if (imp.menu_state == APP_MENU_START) {
    app_menu_state(APP_MENU_OFF);
  }
}

static void app_toggle_pause()
{
  if (imp.menu_state == APP_MENU_OFF) {
    imp.pause = !imp.pause;
    if (imp.pause) {
      app_popup("Pause", 2);
    }
  }
}

static void app_copy_tmp_file(const char* src, char* dst)
{
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
  SDL_snprintf(dst, APP_PATH_SIZE, "%s%s/%s", imp.save_dir, "Roms", app_rom_name(src));
  SDL_CopyFile(src, dst);
#else
  SDL_strlcpy(dst, src, APP_PATH_SIZE);
#endif
}

static void SDLCALL app_file_dialog_cb(void*, const char* const* files, int)
{
  if (files && files[0]) {
    app_copy_tmp_file(files[0], imp.load_rom_path);
  } else {
    imp.load_rom_path[0] = 0;
  }
  SDL_SetAtomicInt(&imp.load_rom_dialog_done, 1);
}

void app_open_file()
{
  if (imp.load_rom_dialog_wait || imp.embed_enabled) {
    return;
  }
  imp.load_rom_dialog_wait = true;
  SDL_DialogFileFilter filters[] = { { "iNES / NES 2.0 ROMs (*.nes)", "nes" }, { "All Files (*.*)", "*" } };
  SDL_ShowOpenFileDialog(app_file_dialog_cb, nullptr, imp.window, filters, 2, nullptr, false);
}

static void app_drop_file(const char* path)
{
  if (path && !imp.embed_enabled) {
    app_copy_tmp_file(path, imp.load_rom_path);
    app_rom_load(imp.load_rom_path);
    app_rom_update();
  }
}

void app_toggle_region()
{
  imp.region = (imp.region + 1) % 4;
  imp.dirty_config = true;
  mn_region_set(imp.region);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
  app_try_resize();
  app_popup(app_region_labels[imp.region], 2);
}

void app_toggle_aspect()
{
  imp.auto_aspect = !imp.auto_aspect;
  imp.dirty_config = true;
  app_try_resize();
  app_popup(app_aspect_labels[imp.auto_aspect], 2);
}

void app_toggle_overscan()
{
  imp.overscan = !imp.overscan;
  imp.dirty_config = true;
  app_try_resize();
  app_popup(app_overscan_labels[imp.overscan], 2);
}

static void app_set_fullscreen(bool enabled)
{
  SDL_SetWindowFullscreen(imp.window, enabled);
  app_ondisplaychange();
  app_popup(app_fullscreen_labels[enabled], 2);
}

void app_toggle_fullscreen()
{
  Uint32 flags = SDL_GetWindowFlags(imp.window);
  app_set_fullscreen((flags & SDL_WINDOW_FULLSCREEN) == 0);
}

static void app_screenshot()
{
#ifndef __EMSCRIPTEN__
  SDL_Surface* screenshot = SDL_RenderReadPixels(imp.renderer, nullptr);
  if (screenshot) {
    char path[APP_PATH_SIZE] = {};
    char ext[32] = "";
    uint num = 0;
    SDL_PathInfo info;
    while (true) {
      SDL_snprintf(ext, sizeof(ext), ".%d.png", num);
      app_save_path(path, "Screenshots", ext);
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

void app_volume_down()
{
  if (imp.volume > 0) {
    --imp.volume;
    imp.dirty_config = true;
  }
  app_popup(app_volume_labels[imp.volume], 2);
  app_volume_update();
}

void app_volume_up()
{
  if (imp.volume < 10) {
    ++imp.volume;
    imp.dirty_config = true;
  }
  app_popup(app_volume_labels[imp.volume], 2);
  app_volume_update();
}

void app_upscale()
{
#ifndef __EMSCRIPTEN__
  if (imp.scale < 8) {
    ++imp.scale;
    imp.dirty_config = true;
  }
  app_popup(app_scale_labels[imp.scale - 1], 2);
  app_try_resize();
#endif
}

void app_downscale()
{
#ifndef __EMSCRIPTEN__
  if (imp.scale > 1) {
    --imp.scale;
    imp.dirty_config = true;
  }
  app_popup(app_scale_labels[imp.scale - 1], 2);
  app_try_resize();
#endif
}

static void app_fast_forward_on()
{
  imp.fast_forward = true;
  SDL_SetAudioStreamFrequencyRatio(imp.stream, imp.fast_forward_scale);
}

static void app_fast_forward_off()
{
  imp.fast_forward = false;
  SDL_SetAudioStreamFrequencyRatio(imp.stream, 1.0f);
}

void app_fast_forward_down()
{
  if (imp.fast_forward_scale > 2) {
    --imp.fast_forward_scale;
    imp.dirty_config = true;
  }
}

void app_fast_forward_up()
{
  if (imp.fast_forward_scale < 5) {
    ++imp.fast_forward_scale;
    imp.dirty_config = true;
  }
}

void app_auto_save_period_down()
{
  imp.auto_save_period = (imp.auto_save_period / 30) * 30;
  if (imp.auto_save_period > 0) {
    imp.auto_save_period -= 30;
    imp.dirty_config = true;
  }
}

void app_auto_save_period_up()
{
  imp.auto_save_period = (imp.auto_save_period / 30) * 30;
  if (imp.auto_save_period < 300) {
    imp.auto_save_period += 30;
    imp.dirty_config = true;
  }
}

void app_toggle_fps()
{
  imp.fps = !imp.fps;
  imp.dirty_config = true;
}

void app_toggle_fps_test()
{
#ifndef __EMSCRIPTEN__
  imp.fps_test = !imp.fps_test;
  SDL_SetRenderVSync(imp.renderer, !imp.fps_test);
  app_popup(app_fps_test_labels[imp.fps_test], 2);
#endif
}

static void app_slot(uint slot)
{
  imp.slot = slot;
  app_popup(app_save_slot_labels[imp.slot], 2);
}

static void app_slot0() { app_slot(0); }
static void app_slot1() { app_slot(1); }
static void app_slot2() { app_slot(2); }
static void app_slot3() { app_slot(3); }
static void app_slot4() { app_slot(4); }
static void app_slot5() { app_slot(5); }
static void app_slot6() { app_slot(6); }
static void app_slot7() { app_slot(7); }
static void app_slot8() { app_slot(8); }
static void app_slot9() { app_slot(9); }

void app_slot_next()
{
  imp.slot = (imp.slot + 1) % 10;
  app_popup(app_save_slot_labels[imp.slot], 2);
}

void app_slot_prev()
{
  imp.slot = (imp.slot + 9) % 10;
  app_popup(app_save_slot_labels[imp.slot], 2);
}

static void app_quit()
{
#ifndef __EMSCRIPTEN__
  imp.quit = true;
#endif
}

static void app_joy1_dpad_up_down() { imp.joy1 |= MN_INPUT_UP; }
static void app_joy1_dpad_up_up() { imp.joy1 &= ~MN_INPUT_UP; }
static void app_joy1_dpad_left_down() { imp.joy1 |= MN_INPUT_LEFT; }
static void app_joy1_dpad_left_up() { imp.joy1 &= ~MN_INPUT_LEFT; }
static void app_joy1_dpad_down_down() { imp.joy1 |= MN_INPUT_DOWN; }
static void app_joy1_dpad_down_up() { imp.joy1 &= ~MN_INPUT_DOWN; }
static void app_joy1_dpad_right_down() { imp.joy1 |= MN_INPUT_RIGHT; }
static void app_joy1_dpad_right_up() { imp.joy1 &= ~MN_INPUT_RIGHT; }
static void app_joy1_a_down() { imp.joy1 |= MN_INPUT_A; }
static void app_joy1_a_up() { imp.joy1 &= ~MN_INPUT_A; }
static void app_joy1_b_down() { imp.joy1 |= MN_INPUT_B; }
static void app_joy1_b_up() { imp.joy1 &= ~MN_INPUT_B; }
static void app_joy1_a_turbo_down() { imp.joy1 |= MN_INPUT_TURBO_A; }
static void app_joy1_a_turbo_up() { imp.joy1 &= ~MN_INPUT_TURBO_A; }
static void app_joy1_b_turbo_down() { imp.joy1 |= MN_INPUT_TURBO_B; }
static void app_joy1_b_turbo_up() { imp.joy1 &= ~MN_INPUT_TURBO_B; }
static void app_joy1_select_down() { imp.joy1 |= MN_INPUT_SELECT; }
static void app_joy1_select_up() { imp.joy1 &= ~MN_INPUT_SELECT; }
static void app_joy1_start_down() { imp.joy1 |= MN_INPUT_START; }
static void app_joy1_start_up() { imp.joy1 &= ~MN_INPUT_START; }

static void app_joy2_dpad_up_down() { imp.joy2 |= MN_INPUT_UP; }
static void app_joy2_dpad_up_up() { imp.joy2 &= ~MN_INPUT_UP; }
static void app_joy2_dpad_left_down() { imp.joy2 |= MN_INPUT_LEFT; }
static void app_joy2_dpad_left_up() { imp.joy2 &= ~MN_INPUT_LEFT; }
static void app_joy2_dpad_down_down() { imp.joy2 |= MN_INPUT_DOWN; }
static void app_joy2_dpad_down_up() { imp.joy2 &= ~MN_INPUT_DOWN; }
static void app_joy2_dpad_right_down() { imp.joy2 |= MN_INPUT_RIGHT; }
static void app_joy2_dpad_right_up() { imp.joy2 &= ~MN_INPUT_RIGHT; }
static void app_joy2_a_down() { imp.joy2 |= MN_INPUT_A; }
static void app_joy2_a_up() { imp.joy2 &= ~MN_INPUT_A; }
static void app_joy2_b_down() { imp.joy2 |= MN_INPUT_B; }
static void app_joy2_b_up() { imp.joy2 &= ~MN_INPUT_B; }
static void app_joy2_a_turbo_down() { imp.joy2 |= MN_INPUT_TURBO_A; }
static void app_joy2_a_turbo_up() { imp.joy2 &= ~MN_INPUT_TURBO_A; }
static void app_joy2_b_turbo_down() { imp.joy2 |= MN_INPUT_TURBO_B; }
static void app_joy2_b_turbo_up() { imp.joy2 &= ~MN_INPUT_TURBO_B; }

const app_action_data app_actions[APP_ACTION_COUNT] = {
  [APP_ACTION_JOY1_A] = { "bind_joy1_a", app_joy1_a_down, app_joy1_a_up },
  [APP_ACTION_JOY1_B] = { "bind_joy1_b", app_joy1_b_down, app_joy1_b_up },
  [APP_ACTION_JOY1_A_TURBO] = { "bind_joy1_a_turbo", app_joy1_a_turbo_down, app_joy1_a_turbo_up },
  [APP_ACTION_JOY1_B_TURBO] = { "bind_joy1_b_turbo", app_joy1_b_turbo_down, app_joy1_b_turbo_up },
  [APP_ACTION_JOY1_START] = { "bind_joy1_start", app_joy1_start_down, app_joy1_start_up },
  [APP_ACTION_JOY1_SELECT] = { "bind_joy1_select", app_joy1_select_down, app_joy1_select_up },
  [APP_ACTION_JOY1_UP] = { "bind_joy1_up", app_joy1_dpad_up_down, app_joy1_dpad_up_up },
  [APP_ACTION_JOY1_DOWN] = { "bind_joy1_down", app_joy1_dpad_down_down, app_joy1_dpad_down_up },
  [APP_ACTION_JOY1_LEFT] = { "bind_joy1_left", app_joy1_dpad_left_down, app_joy1_dpad_left_up },
  [APP_ACTION_JOY1_RIGHT] = { "bind_joy1_right", app_joy1_dpad_right_down, app_joy1_dpad_right_up },
  [APP_ACTION_JOY2_A] = { "bind_joy2_a", app_joy2_a_down, app_joy2_a_up },
  [APP_ACTION_JOY2_B] = { "bind_joy2_b", app_joy2_b_down, app_joy2_b_up },
  [APP_ACTION_JOY2_A_TURBO] = { "bind_joy2_a_turbo", app_joy2_a_turbo_down, app_joy2_a_turbo_up },
  [APP_ACTION_JOY2_B_TURBO] = { "bind_joy2_b_turbo", app_joy2_b_turbo_down, app_joy2_b_turbo_up },
  [APP_ACTION_JOY2_START] = { "bind_joy2_start", app_joy1_start_down, app_joy1_start_up },
  [APP_ACTION_JOY2_SELECT] = { "bind_joy2_select", app_joy1_select_down, app_joy1_select_up },
  [APP_ACTION_JOY2_UP] = { "bind_joy2_up", app_joy2_dpad_up_down, app_joy2_dpad_up_up },
  [APP_ACTION_JOY2_DOWN] = { "bind_joy2_down", app_joy2_dpad_down_down, app_joy2_dpad_down_up },
  [APP_ACTION_JOY2_LEFT] = { "bind_joy2_left", app_joy2_dpad_left_down, app_joy2_dpad_left_up },
  [APP_ACTION_JOY2_RIGHT] = { "bind_joy2_right", app_joy2_dpad_right_down, app_joy2_dpad_right_up },
  [APP_ACTION_MENU] = { "bind_menu", app_toggle_menu, nullptr },
  [APP_ACTION_HELP] = { "bind_help", app_toggle_help, nullptr },
  [APP_ACTION_OPEN] = { "bind_open", app_open_file, nullptr },
  [APP_ACTION_REGION] = { "bind_region", app_toggle_region, nullptr },
  [APP_ACTION_RESET] = { "bind_reset", app_reset, nullptr },
  [APP_ACTION_SAVE] = { "bind_save", app_save, nullptr },
  [APP_ACTION_ASPECT] = { "bind_aspect", app_toggle_aspect, nullptr },
  [APP_ACTION_OVERSCAN] = { "bind_overscan", app_toggle_overscan, nullptr },
  [APP_ACTION_LOAD] = { "bind_load", app_load, nullptr },
  [APP_ACTION_QUIT] = { "bind_quit", app_quit, nullptr },
  [APP_ACTION_FULLSCREEN] = { "bind_fullscreen", app_toggle_fullscreen, nullptr },
  [APP_ACTION_SCREENSHOT] = { "bind_screenshot", app_screenshot, nullptr },
  [APP_ACTION_PAUSE] = { "bind_pause", app_toggle_pause, nullptr },
  [APP_ACTION_SHOW_FPS] = { "bind_show_fps", app_toggle_fps, nullptr },
  [APP_ACTION_MAX_FPS] = { "bind_max_fps", app_toggle_fps_test, nullptr },
  [APP_ACTION_FAST_FORWARD] = { "bind_fast_forward", app_fast_forward_on, app_fast_forward_off },
  [APP_ACTION_SLOT0] = { "bind_slot0", app_slot0, nullptr },
  [APP_ACTION_SLOT1] = { "bind_slot1", app_slot1, nullptr },
  [APP_ACTION_SLOT2] = { "bind_slot2", app_slot2, nullptr },
  [APP_ACTION_SLOT3] = { "bind_slot3", app_slot3, nullptr },
  [APP_ACTION_SLOT4] = { "bind_slot4", app_slot4, nullptr },
  [APP_ACTION_SLOT5] = { "bind_slot5", app_slot5, nullptr },
  [APP_ACTION_SLOT6] = { "bind_slot6", app_slot6, nullptr },
  [APP_ACTION_SLOT7] = { "bind_slot7", app_slot7, nullptr },
  [APP_ACTION_SLOT8] = { "bind_slot8", app_slot8, nullptr },
  [APP_ACTION_SLOT9] = { "bind_slot9", app_slot9, nullptr },
  [APP_ACTION_SLOT_NEXT] = { "bind_slot_next", app_slot_next, nullptr },
  [APP_ACTION_SLOT_PREV] = { "bind_slot_prev", app_slot_prev, nullptr },
  [APP_ACTION_VOLUME_UP] = { "bind_volume_up", app_volume_up, nullptr },
  [APP_ACTION_VOLUME_DOWN] = { "bind_volume_down", app_volume_down, nullptr },
  [APP_ACTION_SCALE_UP] = { "bind_scale_up", app_upscale, nullptr },
  [APP_ACTION_SCALE_DOWN] = { "bind_scale_down", app_downscale, nullptr },
};

static bool app_touch_hit(float ex, float ey, int x, int y, int r)
{
  int dx = ex - x;
  int dy = ey - y;
  return (dx * dx + dy * dy) < (r * r);
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: app_quit(); break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED: app_ondisplaychange(); break;
    case SDL_EVENT_DROP_FILE: app_drop_file(event->drop.data); break;
    // case SDL_EVENT_DISPLAY_ORIENTATION:
    //   app_set_fullscreen(event->display.data1 == SDL_ORIENTATION_LANDSCAPE
    //                      || event->display.data1 == SDL_ORIENTATION_LANDSCAPE_FLIPPED);
    // break;
    case SDL_EVENT_KEY_DOWN: {
      imp.touch_active = false;
      void (*onkeydown)() = app_actions[imp.action_by_scancode[event->key.scancode]].onkeydown;
      if (onkeydown) {
        onkeydown();
      } else {
        app_popup("Press F1 to show keymap", 2);
      }
      break;
    }
    case SDL_EVENT_KEY_UP: {
      void (*onkeyup)() = app_actions[imp.action_by_scancode[event->key.scancode]].onkeyup;
      if (onkeyup) {
        onkeyup();
      }
      app_start_off();
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
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: app_fast_forward_on(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: app_toggle_menu(); break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: app_save(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: app_load(); break;
      }
      if (event->gbutton.which == imp.gamepad_id1) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: app_joy1_dpad_up_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: app_joy1_dpad_left_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: app_joy1_dpad_down_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: app_joy1_dpad_right_down(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: app_joy1_b_down(); break;
          case SDL_GAMEPAD_BUTTON_EAST: app_joy1_a_down(); break;
          case SDL_GAMEPAD_BUTTON_WEST: app_joy1_b_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: app_joy1_a_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_BACK: app_joy1_select_down(); break;
          case SDL_GAMEPAD_BUTTON_START: app_joy1_start_down(); break;
        }
      } else if (event->gbutton.which == imp.gamepad_id2) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: app_joy2_dpad_up_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: app_joy2_dpad_left_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: app_joy2_dpad_down_down(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: app_joy2_dpad_right_down(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: app_joy2_b_down(); break;
          case SDL_GAMEPAD_BUTTON_EAST: app_joy2_a_down(); break;
          case SDL_GAMEPAD_BUTTON_WEST: app_joy2_b_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: app_joy2_a_turbo_down(); break;
          case SDL_GAMEPAD_BUTTON_BACK: app_joy1_select_down(); break;
          case SDL_GAMEPAD_BUTTON_START: app_joy1_start_down(); break;
        }
      }
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: app_fast_forward_off(); break;
      }
      if (event->gbutton.which == imp.gamepad_id1) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: app_joy1_dpad_up_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: app_joy1_dpad_left_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: app_joy1_dpad_down_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: app_joy1_dpad_right_up(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: app_joy1_b_up(); break;
          case SDL_GAMEPAD_BUTTON_EAST: app_joy1_a_up(); break;
          case SDL_GAMEPAD_BUTTON_WEST: app_joy1_b_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: app_joy1_a_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_BACK: app_joy1_select_up(); break;
          case SDL_GAMEPAD_BUTTON_START: app_joy1_start_up(); break;
        }
      } else if (event->gbutton.which == imp.gamepad_id2) {
        switch (event->gbutton.button) {
          case SDL_GAMEPAD_BUTTON_DPAD_UP: app_joy2_dpad_up_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_LEFT: app_joy2_dpad_left_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_DOWN: app_joy2_dpad_down_up(); break;
          case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: app_joy2_dpad_right_up(); break;
          case SDL_GAMEPAD_BUTTON_SOUTH: app_joy2_b_up(); break;
          case SDL_GAMEPAD_BUTTON_EAST: app_joy2_a_up(); break;
          case SDL_GAMEPAD_BUTTON_WEST: app_joy2_b_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_NORTH: app_joy2_a_turbo_up(); break;
          case SDL_GAMEPAD_BUTTON_BACK: app_joy1_select_up(); break;
          case SDL_GAMEPAD_BUTTON_START: app_joy1_start_up(); break;
        }
      }
      app_start_off();
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
      if (app_touch_hit(ex, ey, imp.touch_a_x, imp.touch_a_y, imp.touch_button_ab_radius)) {
        imp.touch_button[APP_TOUCH_A] = id;
        app_joy1_a_down();
      } else if (app_touch_hit(ex, ey, imp.touch_b_x, imp.touch_b_y, imp.touch_button_ab_radius)) {
        imp.touch_button[APP_TOUCH_B] = id;
        app_joy1_b_down();
      } else if (app_touch_hit(ex, ey, imp.touch_start_x, imp.touch_start_y, imp.touch_button_radius)) {
        imp.touch_button[APP_TOUCH_START] = id;
        app_joy1_start_down();
      } else if (app_touch_hit(ex, ey, imp.touch_select_x, imp.touch_select_y, imp.touch_button_radius)) {
        imp.touch_button[APP_TOUCH_SELECT] = id;
        app_joy1_select_down();
      } else if (app_touch_hit(ex, ey, imp.touch_menu_x, imp.touch_menu_y, imp.touch_button_radius)) {
        app_toggle_menu();
      } else if (app_touch_hit(ex, ey, imp.touch_ff_x, imp.touch_ff_y, imp.touch_button_radius)) {
        imp.touch_button[APP_TOUCH_FF] = id;
        app_fast_forward_on();
      } else {
        int dx = ex - imp.touch_dpad_x;
        int dy = ey - imp.touch_dpad_y;
        int d2 = dx * dx + dy * dy;
        int r2 = imp.touch_dpad_radius * imp.touch_dpad_radius;
        int t2 = imp.touch_dpad_threshold * imp.touch_dpad_threshold;
        if (d2 < r2 && d2 > t2) {
          float a = SDL_atan2f(dy, dx);
          if (a >= -0.875f * SDL_PI_F && a < -0.125f * SDL_PI_F) {
            imp.touch_button[APP_TOUCH_UP] = id;
            app_joy1_dpad_up_down();
          }
          if (a >= 0.125f * SDL_PI_F && a < 0.875f * SDL_PI_F) {
            imp.touch_button[APP_TOUCH_DOWN] = id;
            app_joy1_dpad_down_down();
          }
          if (a < -0.625f * SDL_PI_F || a >= 0.625f * SDL_PI_F) {
            imp.touch_button[APP_TOUCH_LEFT] = id;
            app_joy1_dpad_left_down();
          }
          if (a >= -0.375f * SDL_PI_F && a < 0.375f * SDL_PI_F) {
            imp.touch_button[APP_TOUCH_RIGHT] = id;
            app_joy1_dpad_right_down();
          }
        }
      }
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
#ifdef NDEBUG
      app_start_off();
      break;
#endif
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
      if (!imp.touch_active) {
        imp.touch_active = true;
        app_start_off();
        break;
      }
      SDL_FingerID fingerID = event->type == SDL_EVENT_MOUSE_BUTTON_UP ? SDL_TOUCH_MOUSEID : event->tfinger.fingerID;
      if (imp.touch_button[APP_TOUCH_A] == fingerID) {
        imp.touch_button[APP_TOUCH_A] = 0;
        app_joy1_a_up();
      }
      if (imp.touch_button[APP_TOUCH_B] == fingerID) {
        imp.touch_button[APP_TOUCH_B] = 0;
        app_joy1_b_up();
      }
      if (imp.touch_button[APP_TOUCH_START] == fingerID) {
        imp.touch_button[APP_TOUCH_START] = 0;
        app_joy1_start_up();
      }
      if (imp.touch_button[APP_TOUCH_SELECT] == fingerID) {
        imp.touch_button[APP_TOUCH_SELECT] = 0;
        app_joy1_select_up();
      }
      if (imp.touch_button[APP_TOUCH_FF] == fingerID) {
        imp.touch_button[APP_TOUCH_FF] = 0;
        app_fast_forward_off();
      }
      if (imp.touch_button[APP_TOUCH_UP] == fingerID) {
        imp.touch_button[APP_TOUCH_UP] = 0;
        app_joy1_dpad_up_up();
      }
      if (imp.touch_button[APP_TOUCH_DOWN] == fingerID) {
        imp.touch_button[APP_TOUCH_DOWN] = 0;
        app_joy1_dpad_down_up();
      }
      if (imp.touch_button[APP_TOUCH_LEFT] == fingerID) {
        imp.touch_button[APP_TOUCH_LEFT] = 0;
        app_joy1_dpad_left_up();
      }
      if (imp.touch_button[APP_TOUCH_RIGHT] == fingerID) {
        imp.touch_button[APP_TOUCH_RIGHT] = 0;
        app_joy1_dpad_right_up();
      }
      break;
    }
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
      if (app_touch_hit(ex, ey, imp.touch_a_x, imp.touch_a_y, imp.touch_button_ab_radius)) {
        if (imp.touch_button[APP_TOUCH_A] != id) {
          imp.touch_button[APP_TOUCH_A] = id;
          app_joy1_a_down();
        }
      } else if (imp.touch_button[APP_TOUCH_A] == id) {
        imp.touch_button[APP_TOUCH_A] = 0;
        app_joy1_a_up();
      }
      if (app_touch_hit(ex, ey, imp.touch_b_x, imp.touch_b_y, imp.touch_button_ab_radius)) {
        if (imp.touch_button[APP_TOUCH_B] != id) {
          imp.touch_button[APP_TOUCH_B] = id;
          app_joy1_b_down();
        }
      } else if (imp.touch_button[APP_TOUCH_B] == id) {
        imp.touch_button[APP_TOUCH_B] = 0;
        app_joy1_b_up();
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
          if (imp.touch_button[APP_TOUCH_UP] != id) {
            imp.touch_button[APP_TOUCH_UP] = id;
            app_joy1_dpad_up_down();
          }
        } else if (imp.touch_button[APP_TOUCH_UP] == id) {
          imp.touch_button[APP_TOUCH_UP] = 0;
          app_joy1_dpad_up_up();
        }
        if (hit && a >= 0.125f * SDL_PI_F && a < 0.875f * SDL_PI_F) {
          if (imp.touch_button[APP_TOUCH_DOWN] != id) {
            imp.touch_button[APP_TOUCH_DOWN] = id;
            app_joy1_dpad_down_down();
          }
        } else if (imp.touch_button[APP_TOUCH_DOWN] == id) {
          imp.touch_button[APP_TOUCH_DOWN] = 0;
          app_joy1_dpad_down_up();
        }
        if (hit && (a < -0.625f * SDL_PI_F || a >= 0.625f * SDL_PI_F)) {
          if (imp.touch_button[APP_TOUCH_LEFT] != id) {
            imp.touch_button[APP_TOUCH_LEFT] = id;
            app_joy1_dpad_left_down();
          }
        } else if (imp.touch_button[APP_TOUCH_LEFT] == id) {
          imp.touch_button[APP_TOUCH_LEFT] = 0;
          app_joy1_dpad_left_up();
        }
        if (hit && a >= -0.375f * SDL_PI_F && a < 0.375f * SDL_PI_F) {
          if (imp.touch_button[APP_TOUCH_RIGHT] != id) {
            imp.touch_button[APP_TOUCH_RIGHT] = id;
            app_joy1_dpad_right_down();
          }
        } else if (imp.touch_button[APP_TOUCH_RIGHT] == id) {
          imp.touch_button[APP_TOUCH_RIGHT] = 0;
          app_joy1_dpad_right_up();
        }
      }
      break;
    }
  }
  return imp.quit ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

#include "myaownes.h"
#include "app.h"
#include "actions.h"
#include "views.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static void imp_init()
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
}

SDL_AppResult SDL_AppInit(void**, int argc, char* argv[])
{
  SDL_Log("MyaowNES v%s", MN_VERSION);
  SDL_memset(&imp, 0, sizeof(imp));

  for (int i = 0; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "-h") == 0 || SDL_strcmp(argv[i], "--help") == 0) {
      SDL_Log("MyaowNES v" MN_VERSION);
      SDL_Log("Usage: myaownes <path/to/game.nes> [options]");
      SDL_Log("Options:");
      SDL_Log("-h, --help                       Display help");
      SDL_Log("--embed                          Embed mode");
      SDL_Log("--config   <path/to/config.txt>  Default config");
      SDL_Log("--path     <path/to/save/dir>    Path to save directory");
      return SDL_APP_SUCCESS;
    } else if (SDL_strcmp(argv[i], "--embed") == 0) {
      imp.embed_enabled = true;
    } else if (SDL_strcmp(argv[i], "--config") == 0) {
      SDL_strlcpy(imp.default_config_path, argv[++i], IMP_PATH_SIZE);
    } else if (SDL_strcmp(argv[i], "--path") == 0) {
      SDL_strlcpy(imp.save_dir, argv[++i], IMP_PATH_SIZE);
      uint len = SDL_strlen(imp.save_dir);
      if (len > 0 && imp.save_dir[len - 1] != '/' && imp.save_dir[len - 1] != '\\') {
        SDL_strlcat(imp.save_dir, "/", IMP_PATH_SIZE);
      }
    } else if (i > 0) {
      SDL_strlcpy(imp.load_rom_path, argv[i], IMP_PATH_SIZE);
    }
  }

  if (imp.save_dir[0] == 0) {
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

  if (imp.load_rom_path[0]) {
    imp_rom_load(imp.load_rom_path);
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

  imp_init();
  imp_rom_update();

  if (imp.embed_enabled && imp.start_screen && imp_audio_suspended()) {
    imp_menu_state(IMP_MENU_START);
  }

  return SDL_APP_CONTINUE;
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
      imp_start_off();
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
      imp_start_off();
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
    case SDL_EVENT_MOUSE_BUTTON_UP:
#ifdef NDEBUG
      imp_start_off();
      break;
#endif
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
      if (!imp.touch_active) {
        imp.touch_active = true;
        imp_start_off();
        break;
      }
      SDL_FingerID fingerID = event->type == SDL_EVENT_MOUSE_BUTTON_UP ? SDL_TOUCH_MOUSEID : event->tfinger.fingerID;
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
  SDL_CloseGamepad(imp.gamepad1);
  SDL_CloseGamepad(imp.gamepad2);
  SDL_DestroyAudioStream(imp.stream);
  SDL_CloseAudioDevice(imp.audioDeviceId);
  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

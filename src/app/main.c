#include "myaownes.h"
#include "app.h"
#include "events.h"
#include "views.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

app imp;

static void app_init()
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
    app_rom_load(rom);
  }
  if (tas) {
    app_tas_load(tas);
  }
}

static void app_ensure_dir(const char* subdir)
{
  char path[APP_PATH_SIZE] = {};
  SDL_strlcat(path, imp.save_dir, APP_PATH_SIZE);
  SDL_strlcat(path, subdir, APP_PATH_SIZE);
  SDL_CreateDirectory(path);
}

static bool app_audio_suspended()
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

  for (int i = 0; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "-h") == 0 || SDL_strcmp(argv[i], "--help") == 0) {
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
      SDL_strlcpy(imp.default_config_path, argv[++i], APP_PATH_SIZE);
    } else if (SDL_strcmp(argv[i], "--path") == 0) {
      SDL_strlcpy(imp.save_dir, argv[++i], APP_PATH_SIZE);
      uint len = SDL_strlen(imp.save_dir);
      if (len > 0 && imp.save_dir[len - 1] != '/' && imp.save_dir[len - 1] != '\\') {
        SDL_strlcat(imp.save_dir, "/", APP_PATH_SIZE);
      }
    } else if (i > 0) {
      SDL_strlcpy(imp.load_rom_path, argv[i], APP_PATH_SIZE);
    }
  }

  if (imp.save_dir[0] == 0) {
    char* pref_path = SDL_GetPrefPath(nullptr, "MyaowNES");
    SDL_strlcpy(imp.save_dir, pref_path, APP_PATH_SIZE);
    SDL_free(pref_path);
  }

  SDL_Log("Save path: %s", imp.save_dir);

  app_ensure_dir("Saves");
  app_ensure_dir("Screenshots");
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
  app_ensure_dir("Roms");
#endif

  SDL_strlcat(imp.config_path, imp.save_dir, APP_PATH_SIZE);
  SDL_strlcat(imp.config_path, "config.txt", APP_PATH_SIZE);

  SDL_strlcat(imp.history_path, imp.save_dir, APP_PATH_SIZE);
  SDL_strlcat(imp.history_path, "history.txt", APP_PATH_SIZE);

  app_load_config();
  app_load_history();
  mn_region_set(imp.region);

  if (imp.load_rom_path[0]) {
    app_rom_load(imp.load_rom_path);
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

  if (!app_load_atlas()) {
    return SDL_APP_FAILURE;
  }

  imp.audioDeviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 48000 };
  int samples = 2048;
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
  app_volume_update();

  app_init();
  app_rom_update();

  if (imp.embed_enabled && imp.start_screen && app_audio_suspended()) {
    app_menu_state(APP_MENU_START);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
  if (imp.load_rom_dialog_wait) {
    if (SDL_CompareAndSwapAtomicInt(&imp.load_rom_dialog_done, 1, 0)) {
      imp.load_rom_dialog_wait = false;
      if (imp.load_rom_path[0]) {
        app_rom_load(imp.load_rom_path);
        app_rom_update();
      }
    }
  }

  Uint64 time = SDL_GetPerformanceCounter();
  if (imp.last_time == 0) {
    imp.last_time = time;
  }
  imp.real_frame_time = time - imp.last_time;
  imp.last_time = time;

  if (!imp.pause) {
    Uint64 game_frame_time = imp.real_frame_time;
    if (imp.fast_forward) {
      game_frame_time *= imp.fast_forward_scale;
      app_popup(">>", 0.5);
    } else if (imp.fps_test) {
      game_frame_time = imp.target_frame_time * 32;
    }

    imp.curr_time += game_frame_time;

    uint max_time = imp.target_frame_time * ((imp.fast_forward ? imp.fast_forward_scale : 1) + 2);
    if (imp.curr_time > max_time && !imp.fps_test) {
      imp.curr_time = max_time;
    }

    uint queued_size = SDL_GetAudioStreamQueued(imp.stream);
    bool has_video = false;
    bool has_audio = queued_size >= imp.audio_buffer_size;

    if (imp.tas) {
      mn_tas_input(imp.tas);
    } else {
      mn_input(imp.joy1, imp.joy2);
    }

    while (imp.curr_time >= imp.target_frame_time || !has_audio) {
      imp.curr_time = imp.curr_time > imp.target_frame_time ? imp.curr_time - imp.target_frame_time : 0;
      if (imp.fps) {
        ++imp.fps_count;
      }
      if (!has_video || !has_audio) {
        mn_frame();
        uint audio_size = mn_audio_size();
        uint target_size = imp.audio_buffer_size > audio_size ? imp.audio_buffer_size : audio_size;
        if (queued_size < 3 * target_size) {
          SDL_PutAudioStreamData(imp.stream, mn_audio_data(), audio_size);
          queued_size += audio_size;
          has_audio = queued_size >= imp.audio_buffer_size;
        }
        has_video = true;
      } else {
        mn_frame_fast();
      }
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
  app_save_config();

  SDL_SetRenderScale(imp.renderer, 1, 1);
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xFF);
  SDL_RenderClear(imp.renderer);

  SDL_UpdateTexture(imp.tex_out, nullptr, mn_video_data(), 256 * 4);
  SDL_RenderTexture(imp.renderer, imp.tex_out, nullptr, &imp.rect);

  SDL_SetRenderScale(imp.renderer, imp.ui_offset.w, imp.ui_offset.h);
  switch (imp.menu_state) {
    case APP_MENU_MAIN: app_draw_menu(); break;
    case APP_MENU_OPTIONS: app_draw_options(); break;
    case APP_MENU_INFO: app_draw_info(); break;
    case APP_MENU_HELP: app_draw_help(); break;
    case APP_MENU_RECENT: app_draw_recent(); break;
    case APP_MENU_START: app_draw_start(); break;
  }
  if (imp.popup_time > 0) {
    app_draw_popup();
  }
  if (imp.fps) {
    app_draw_fps();
  }
  if (imp.touch_active) {
    SDL_SetRenderScale(imp.renderer, 1, 1);
    app_draw_touch();
  }

  SDL_RenderPresent(imp.renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult)
{
  app_rom_unload();
  app_tas_unload();
  app_save_config();
  SDL_CloseGamepad(imp.gamepad1);
  SDL_CloseGamepad(imp.gamepad2);
  SDL_DestroyAudioStream(imp.stream);
  SDL_CloseAudioDevice(imp.audioDeviceId);
  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyTexture(imp.tex_buttons);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

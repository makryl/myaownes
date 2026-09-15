#include "myaownes.h"
#include "common.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

#include <unistd.h>

static void imp_rom_load(const char* path);

static bool imp_init()
{
  const char* test = nullptr;

  // test = "nes-test-roms/other/nestest.nes";
  // test = "nes-test-roms/240pee/240pee.nes";
  // test = "nes-test-roms/other/AccuracyCoin.nes";
  // test = "nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes";
  // test = "nes-test-roms/nmi_sync/demo_ntsc.nes";
  // test = "nes-test-roms/nmi_sync/demo_pal.nes";
  // test = "nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes";
  // test = "nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes";
  // test = "nes-test-roms/apu_test/rom_singles/7-dmc_basics.nes";
  // test = "nes-test-roms/apu_test/rom_singles/8-dmc_rates.nes";
  // test = "nes-test-roms/tvpassfail/tv.nes"; // todo: chroma/luma?
  // test = "nes-test-roms/other/8bitpeoples_-_deadline_console_invitro.nes";
  // test = "nes-test-roms/other/n163test.nes"; // todo fail audio
  // test = "nes-test-roms/other/sprite_evaluation_test(2).nes"; // todo
  // test = "nes-test-roms/other/sprite_evaluation_test.nes"; // todo
  // test = "nes-test-roms/cpu_flag_concurrency/test_cpu_flag_concurrency.nes"; // 4015 open bus bit?

  // test = "GoodNES/USA/Mega Man (U) [!].nes";
  // test = "GoodNES/USA/Contra (U) [!].nes";
  // test = "GoodNES/Japan/Fire Emblem Gaiden (J) [!].nes";
  // test = "MegaPack/Translated/Fire Emblem Gaiden (J) [T-Eng97b2].nes";

  // test = "no-intro-2025/Games/Battletoads (USA).nes";
  // test = "no-intro-2025/Games/Micro Machines (USA) (Unl).nes";
  // test = "GoodNES/USA/Marble Madness (U) [!].nes";
  // test = "no-intro-2025/Games/Crystalis (USA).nes";
  // test = "no-intro-2025/Super Mario Bros. + Duck Hunt (USA).nes";
  // test = "GoodNES/USA/Super Mario Bros. 3 (U) (V1.1) [!].nes";
  // test = "no-intro-2025/Games/Rad Racer (USA).nes";
  // test = "GoodNES/USA/Legend of Zelda, The (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Kirby's Adventure (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Immortal, The (U) [!].nes";

  // test = "no-intro-2025/Games/Super Mario Bros. (Europe).nes";
  // test = "GoodNES/USA/Elite (U) (Proto) [!].nes";
  // test = "GoodNES/Europe/Elite (E) [!].nes";
  // test = "GoodNES/Europe/Asterix (E) [!].nes";
  // test = "GoodNES/Europe/Smurfs, The (E) [!].nes";
  // test = "GoodNES/USA/Batman - Return of the Joker (U) [!].nes"; // FME-7
  // test = "GoodNES/Japan/Magic John (J) [!].nes"; // Jaleco SS
  // test = "GoodNES/Japan/TwinBee 3 - Poko Poko Daimaou (J) [!].nes"; // VRC2a	22 +
  // test = "GoodNES/Japan/Konami Wai Wai World (J) [!].nes"; // VRC2b	23-3 +
  // test = "GoodNES/Japan/Contra (J) [!].nes"; // VRC2b	23-3 +
  // test = "GoodNES/Japan/Ganbare Goemon Gaiden - Kieta Ougon Kiseru (J) (V1.1) [!].nes"; // VRC2c	25-3 +
  // test = "GoodNES/Japan/Wai Wai World 2 - SOS!! Paseri Jou (J) [!].nes"; // VRC4a	21-1 +
  // test = "GoodNES/Japan/Bio Miracle Bokutte Upa (J) [!].nes"; // VRC4b	25-1 +
  // test = "GoodNES/Japan/Ganbare Goemon Gaiden 2 - Tenka no Zaihou (J) [!].nes"; // VRC4c 21-2 +
  // test = "GoodNES/Japan/Teenage Mutant Ninja Turtles (J) [!].nes"; // VRC4d	25-2 +
  // test = "GoodNES/Japan/Akumajou Special - Boku Dracula-kun (J) [!].nes"; // VRC4e	23-2 +
  // test = "GoodNES/Japan/Tiny Toon Adventures (J) [!].nes"; // VRC4e	23-2 +
  // test = "GoodNES/Japan/Akumajou Densetsu (J) [!].nes"; // VRC6a +
  // test = "GoodNES/Japan/Esper Dream 2 - Aratanaru Tatakai (J) [!].nes"; // VRC6b +
  // test = "GoodNES/Japan/Mouryou Senki Madara (J) [!].nes"; // VRC6b +
  // test = "GoodNES/Japan/Salamander (J) [!].nes"; // VRC3 +
  // test = "GoodNES/Japan/Ganbare Goemon! - Karakuri Douchuu (J) [!].nes"; // VRC1 +
  // test = "GoodNES/Japan/Tetsuwan Atom (J) [!].nes"; // VRC1 +
  // test = "GoodNES/Japan/Lagrange Point (J) [!].nes"; // VRC7a +
  // test = "GoodNES/Japan/Tiny Toon Adventures 2 - Montana Land e Youkoso (J) [!].nes"; // VRC7b +
  // test = "GoodNES/Japan/Fantasy Zone II - Opa-Opa no Namida (J) [!].nes"; // Sunsoft-3
  // test = "GoodNES/Japan/Mappy Kids (J) [!].nes"; // Namco-163
  // test = "MegaPack/Translated/Splatter House - Wanpaku Graffiti (J) [T-Eng2.0].nes"; // Namco-163
  // test = "GoodNES/Japan/Digital Devil Story - Megami Tensei II (J) (V1.1) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/King of Kings (J) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Star Wars (J) (Namco) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Dokuganryuu Masamune (J) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Family Circuit '91 (J) [!].nes"; // Namco-175
  // test = "GoodNES/Japan/Splatterhouse - Wanpaku Graffiti (J) [!].nes"; // Namco-340
  // test = "GoodNES/Japan/Digital Devil Story - Megami Tensei (J) [!].nes"; // Namco-3446
  // test = "GoodNES/Japan/Babel no Tou (J) (V1.0) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Family Jockey (J) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Metro-Cross (J) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Quinty (J) [!].nes"; // Namco-3433
  // test = "GoodNES/Japan/Devil Man (J) [!].nes"; // Namco-3453
  // test = "GoodNES/Japan/Dragon Buster (J) [!].nes"; // Namco-3425
  // test = "GoodNES/Japan/Akuma-kun - Makai no Wana (J) [!].nes"; // Bandai FCG-2
  // test = "GoodNES/Japan/Crayon Shin-chan - Ora to Poi Poi (J) [!].nes"; // Bandai LZ93D50
  // test = "GoodNES/Japan/Dragon Ball - Daimaou Fukkatsu (J) [!].nes"; // Bandai FCG-1
  // test = "GoodNES/Japan/Dragon Ball 3 - Gokuu Den (J) (V1.1) [!].nes"; // Bandai FCG-2
  // test = "GoodNES/Japan/Dragon Ball Z II - Gekishin Freeza!! (J) (V1.1) [!].nes"; // Bandai LZ93D50 EEPROM 256
  // test = "GoodNES/Japan/Dragon Ball Z - Kyoushuu! Saiya Jin (J) (V1.1) [!].nes"; // Bandai LZ93D50 EEPROM 128
  // test = "GoodNES/Japan/Famicom Jump II - Saikyou no 7 Nin (J) [!].nes"; // Bandai LZ93D50 WRAM
  // test = "MegaPack/USA/Klax (U).nes"; // Tengen Rambo-1
  // test = "MegaPack/USA/Skull & Crossbones (U).nes"; // Tengen Rambo-1
  // test = "MegaPack/USA/Shinobi (U).nes"; // Tengen Rambo-1
  // test = "no-intro-2025/Games/Alien Syndrome (USA) (Unl).nes"; // Tengen Rambo-1 800037
  // test = "no-intro-2025/Games/After Burner (USA) (Unl).nes"; // Sunsoft-4
  // test = "no-intro-2025/Maharaja (Japan).nes"; // Sunsoft-4
  // test = "GoodNES/Japan/Nantettatte!! Baseball (J) [!].nes"; // Sunsoft-4
  // test
  //   = "GoodNES/Japan/Nantettatte!! Baseball + Nantettatte!! Baseball - Ko-Game Cassette - OB All Star Hen (J)
  //   [!].nes"; // Sunsoft-4 with external rom
  // test
  //   = "GoodNES/Japan/Nantettatte!! Baseball + Nantettatte!! Baseball - Ko-Game Cassette - '91 Kaimaku Hen (J)
  //   [!].nes"; // Sunsoft-4 with external rom
  // test = "no-intro-2025/Don Doko Don (Japan).nes"; // Taito tc0190
  // test = "no-intro-2025/Games/Don Doko Don 2 (Japan).nes"; // Taito tc0690
  // test = "no-intro-2025/Games/Flintstones, The - The Rescue of Dino & Hoppy (Japan).nes"; // Taito tc0690
  // test = "no-intro-2025/Games/Jetsons, The - Cogswell's Caper (Japan).nes"; // Taito tc0690 (submapper 1)
  // test = "no-intro-2025/Kyonshiizu 2 (Japan).nes"; // Taito x1-005
  // test = "no-intro-2025/Games/Fudou Myouou Den (Japan).nes"; // Taito x1-005a
  // test = "no-intro-2025/Games/SD Keiji - Blader (Japan).nes"; // Taito x1-017
  // test = "no-intro-2025/Kyuukyoku Harikiri Stadium III (Japan).nes"; // Taito x1-017
  // test = "no-intro-2025/Games/Kyuukyoku Harikiri Koushien (Japan).nes"; // Taito x1-017
  // test = "no-intro-2025/Kaiketsu Yanchamaru 2 - Karakuri Land (Japan).nes"; // Irem G-101
  // test = "no-intro-2025/Major League (Japan).nes"; // Irem G-101 (submapper 1)
  // test = "no-intro-2025/Kaiketsu Yanchamaru (Japan).nes"; // Irem TAM-S1
  // test = "no-intro-2025/Games/Daiku no Gen-san 2 - Akage no Dan no Gyakushuu (Japan).nes"; // Irem H3001
  // test = "no-intro-2025/Kaiketsu Yanchamaru 3 - Taiketsu! Zouringen (Japan).nes"; // Irem H3001
  // test = "no-intro-2025/Moero!! Pro Yakyuu (Japan) (Rev 3).nes"; // Jaleco JF-13
  // test = "no-intro-2025/Games/Dragon Ball Z 5 (Taiwan) (En,Zh-Hant) (Pirate).nes"; // MMC3A Huang-1
  // test = "no-intro-2025/2A03 Puritans (World) (Aftermarket) (Unl).nes"; // NSF Subset

  if (test) {
    chdir("../../tmp");
    imp_rom_load(test);
  }

  return true;
}

enum
{
  IMP_PATH_SIZE = 4096,
};

static struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* tex_out;
  SDL_Texture* tex_font;
  SDL_Gamepad* gamepad1;
  SDL_Gamepad* gamepad2;
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
  uint auto_save_period;
  uint scale;
  uint turbo;
  uint slot;
  uint audio_buffer_size;
  uint fast_forward_scale;
  uint region;
  uint joy1_mask;
  uint joy2_mask;
  uint joy1;
  uint joy2;
  bool pause;
  bool help;
  bool fps;
  bool fps_test;
  bool fast_forward;
  bool auto_aspect;
  bool overscan;
  bool joy1_turbo_a;
  bool joy1_turbo_b;
  bool joy2_turbo_a;
  bool joy2_turbo_b;
  bool dirty_config;
  mn_rom rom;
  char rom_info[6][15];
  char rom_path[IMP_PATH_SIZE];
  char sram_path[IMP_PATH_SIZE];
  char save_dir[IMP_PATH_SIZE];
  char config_path[IMP_PATH_SIZE];
  char palette_path[IMP_PATH_SIZE];
} imp;

static const char* const imp_save_slot_labels[10] = {
  "Slot 0", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5", "Slot 6", "Slot 7", "Slot 8", "Slot 9",
};

static void imp_draw_help()
{
  float x = imp.ui_offset.x;
  float y = imp.ui_offset.y;
  SDL_FRect fill = { x, y, 256, 240 };
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xC0);
  SDL_SetRenderDrawBlendMode(imp.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(imp.renderer, &fill);

  uint i = 2;
  SDL_SetRenderDrawColor(imp.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Help        F1        MyaowNES ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Open DnDrop,F2          v" MN_VERSION " ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Region      F3                 ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Reset       F4    /\\____/\\     ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Quick save  F5                 ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Aspect      F6   |  o..o  |    ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Overscan    F7   |=<+__+>=|    ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Quick load  F8   |        |    ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Quit       F10   [_m____m_]    ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Fullscreen F11                 ");
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " Screenshot F12  %s ", imp.rom_info[0]);
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " Save slot  0-9  %s ", imp.rom_info[1]);
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " F-forward  Tab  %s ", imp.rom_info[2]);
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " Pause      Esc  %s ", imp.rom_info[3]);
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " Scale      -/+  %s ", imp.rom_info[4]);
  SDL_RenderDebugTextFormat(imp.renderer, x, y + 8 * i++, " FPS          `  %s ", imp.rom_info[5]);
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, "                                ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, "           Joy1    Joy2          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " D-pad     WASD  Arrows          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " B            J   NUM_1          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " A            K   NUM_2          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " B turbo      U   NUM_4          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " A turbo      I   NUM_5          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Start        H   Enter          ");
  SDL_RenderDebugText(imp.renderer, x, y + 8 * i++, " Select       F   Space          ");
}

static void imp_draw_popup()
{
  imp.popup_time -= imp.popup_time > imp.real_frame_time ? imp.real_frame_time : imp.popup_time;
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 2, imp.ui_offset.y + 13, imp.popup_text);
  SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
  SDL_RenderDebugText(imp.renderer, imp.ui_offset.x + 1, imp.ui_offset.y + 12, imp.popup_text);
}

static void imp_popup(const char* text, float time)
{
  imp.popup_text = text;
  imp.popup_time = imp.perf_freq * time;
}

static void imp_update_frame_time()
{
  imp.target_frame_time = imp.perf_freq / ((mn_region_get() == MN_REGION_NTSC) ? 60 : 50);
}

static void imp_onresize()
{
  int width;
  int height;
  SDL_GetWindowSizeInPixels(imp.window, &width, &height);
  mn_video_fit(imp.auto_aspect, imp.overscan, width, height, &imp.rect.x, &imp.rect.y, &imp.rect.w, &imp.rect.h);
  imp.ui_offset.w = imp.rect.w / 256;
  imp.ui_offset.h = imp.rect.h / 240;
  imp.ui_offset.x = imp.rect.x / imp.ui_offset.w;
  imp.ui_offset.y = imp.rect.y / imp.ui_offset.h;
}

static void imp_try_resize()
{
#ifndef __EMSCRIPTEN__
  uint width;
  uint height;
  mn_video_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);
  SDL_SetWindowSize(imp.window, width, height);
#endif
  imp_onresize();
}

static void imp_ensure_dir(const char* subdir)
{
  char path[IMP_PATH_SIZE] = {};
  SDL_strlcat(path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(path, subdir, IMP_PATH_SIZE);
  SDL_CreateDirectory(path);
}

static const char* imp_rom_name()
{
  const char* name = imp.rom_path;
  const char* slash = SDL_strrchr(imp.rom_path, '/');
  const char* backslash = SDL_strrchr(imp.rom_path, '\\');
  if (slash || backslash) {
    name = (slash > backslash ? slash : backslash) + 1;
  }
  return name;
}

static void imp_save_path(char* dst, const char* subdir, const char* ext)
{
  dst[0] = 0;
  SDL_snprintf(dst, IMP_PATH_SIZE, "%s%s/%s", imp.save_dir, subdir, imp_rom_name());
  char* dot = SDL_strrchr(dst, '.');
  if (dot) {
    *dot = 0;
  }
  SDL_strlcat(dst, ext, IMP_PATH_SIZE);
}

static void imp_rom_update()
{
  mn_rom rom = imp.rom;
  imp.help = !rom || rom->mapper_error;
  imp.pause = !rom || rom->mapper_error;
  if (!rom) {
    for (uint i = 0; i < 6; ++i) {
      imp.rom_info[i][0] = 0;
    }
    return;
  }

  char title[256] = {};
  SDL_strlcat(title, imp_rom_name(), IMP_PATH_SIZE);
  SDL_strlcat(title, " - MyaowNES", IMP_PATH_SIZE);

  SDL_SetWindowTitle(imp.window, title);

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

  imp_update_frame_time();
  imp_try_resize();
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
  fprintf(f, "scale %u\n", imp.scale);
  fprintf(f, "auto_aspect %u\n", imp.auto_aspect);
  fprintf(f, "overscan %u\n", imp.overscan);
  fprintf(f, "auto_save_period %u\n", imp.auto_save_period);
  fprintf(f, "fps %u\n", imp.fps);
  fprintf(f, "fast_forward_scale %u\n", imp.fast_forward_scale);
  fprintf(f, "region %u\n", imp.region);
  fprintf(f, "palette %s\n", imp.palette_path);
  fclose(f);
  imp.dirty_config = false;
}

static void imp_load_config()
{
  imp.scale = 2;
  imp.auto_aspect = true;
  imp.overscan = false;
  imp.auto_save_period = 60;
  imp.fps = false;
  imp.fast_forward_scale = 4;
  imp.region = MN_REGION_AUTO;

  FILE* f = fopen(imp.config_path, "r");
  if (!f) {
    return;
  }
  char key[32];
  while (fscanf(f, "%31s", key) == 1) {
    if (strcmp(key, "scale") == 0) {
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
    } else if (strcmp(key, "fps") == 0) {
      uint val;
      fscanf(f, "%u", &val);
      imp.fps = val;
    } else if (strcmp(key, "fast_forward_scale") == 0) {
      fscanf(f, "%u", &imp.fast_forward_scale);
    } else if (strcmp(key, "region") == 0) {
      fscanf(f, "%u", &imp.region);
    } else if (strcmp(key, "palette") == 0) {
      fscanf(f, " %[^\n]", imp.palette_path);
      if (strlen(imp.palette_path)) {
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
    }
  }
  fclose(f);
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
  imp_rom_unload();
  SDL_strlcpy(imp.rom_path, path, IMP_PATH_SIZE);
  imp_save_path(imp.sram_path, "Saves", ".sav");
  imp.rom = mn_rom_load(imp.rom_path, imp.sram_path);
  if (imp.rom) {
    mn_rom_set(imp.rom);
  }
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

static void imp_load()
{
  char qs_path[IMP_PATH_SIZE] = {};
  char ext[5] = ".qs0";
  ext[3] += imp.slot;
  imp_save_path(qs_path, "Saves", ext);
  if (mn_load(qs_path)) {
    imp_popup("Loaded", 2);
  }
}

static void SDLCALL imp_file_dialog_cb(void*, const char* const* files, int)
{
  if (!files || !*files) {
    return;
  }
  imp_rom_load(files[0]);
  imp_rom_update();
}

SDL_AppResult SDL_AppInit(void**, int argc, char* argv[])
{
  SDL_Log("MyaowNES v%s", MN_VERSION);
  SDL_memset(&imp, 0, sizeof(imp));

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

  if (SDL_strlen(imp.save_dir) == 0) {
    char* pref_path = SDL_GetPrefPath(nullptr, "MyaowNES");
    SDL_strlcpy(imp.save_dir, pref_path, IMP_PATH_SIZE);
    SDL_free(pref_path);
  }

  SDL_Log("Save path: %s", imp.save_dir);

  imp_ensure_dir("Saves");
  imp_ensure_dir("Screenshots");

  SDL_strlcat(imp.config_path, imp.save_dir, IMP_PATH_SIZE);
  SDL_strlcat(imp.config_path, "config.txt", IMP_PATH_SIZE);

  imp_load_config();
  mn_region_set(imp.region);

  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#ifdef __EMSCRIPTEN__
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
#else
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
#endif

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  imp.perf_freq = SDL_GetPerformanceFrequency();

  uint width;
  uint height;
  mn_video_size(imp.auto_aspect, imp.overscan, imp.scale, &width, &height);

  if (!SDL_CreateWindowAndRenderer("MyaowNES", width, height, SDL_WINDOW_RESIZABLE, &imp.window, &imp.renderer)) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_DisableScreenSaver();

  int vsync;
  SDL_GetRenderVSync(imp.renderer, &vsync);
  SDL_Log("Video: %s %s %s", SDL_GetRendererName(imp.renderer), vsync ? "vsync" : "no-vsync",
          SDL_ScreenSaverEnabled() ? "screensaver" : "no-screensaver");

  imp.tex_out = SDL_CreateTexture(imp.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!imp.tex_out) {
    SDL_Log("%s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetTextureScaleMode(imp.tex_out, SDL_SCALEMODE_PIXELART);

  SDL_AudioSpec spec;
  int samples;
  SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &samples);
  SDL_AudioSpec stream_spec = { SDL_AUDIO_S16, 1, spec.freq };
  imp.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &stream_spec, nullptr, nullptr);
  SDL_ResumeAudioStreamDevice(imp.stream);
  SDL_Log("Audio: %s %d %s%d%s %dch %d", SDL_GetCurrentAudioDriver(), spec.freq,
          SDL_AUDIO_ISFLOAT(spec.format)      ? "F"
          : SDL_AUDIO_ISUNSIGNED(spec.format) ? "U"
                                              : "S",
          SDL_AUDIO_BITSIZE(spec.format), SDL_AUDIO_ISBIGENDIAN(spec.format) ? "BE" : "LE", spec.channels, samples);
  imp.audio_buffer_size = samples * sizeof(i16);
  mn_audio_freq(spec.freq);

  bool inited = imp_init();
  imp_rom_update();

  return inited ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

static void imp_help()
{
  if (imp.rom) {
    if (imp.help) {
      imp.help = false;
      imp.pause = false;
    } else {
      imp.help = true;
      imp.pause = true;
      imp_draw_help();
    }
  }
}

static void imp_open_file()
{
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
  switch (imp.region) {
    case MN_REGION_NTSC: imp_popup("Region NTSC", 2); break;
    case MN_REGION_PAL: imp_popup("Region PAL", 2); break;
    case MN_REGION_AUTO: imp_popup("Region Auto", 2); break;
    case MN_REGION_DENDY: imp_popup("Region Dendy", 2); break;
  }
}

static void imp_toggle_aspect()
{
  imp.auto_aspect = !imp.auto_aspect;
  imp.dirty_config = true;
  imp_try_resize();
  imp_popup(imp.auto_aspect ? "Aspect ON" : "Aspect OFF", 2);
}

static void imp_toggle_overscan()
{
  imp.overscan = !imp.overscan;
  imp.dirty_config = true;
  imp_try_resize();
  imp_popup(imp.overscan ? "Overscan ON" : "Overscan OFF", 2);
}

static void imp_toggle_fullscreen()
{
  Uint32 flags = SDL_GetWindowFlags(imp.window);
  bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
  SDL_SetWindowFullscreen(imp.window, !is_fullscreen);
}

static void imp_screenshot()
{
  SDL_Surface* screenshot = SDL_RenderReadPixels(imp.renderer, nullptr);
  if (screenshot) {
    char path[IMP_PATH_SIZE] = {};
    char ext[32] = "";
    uint num = 0;
    SDL_PathInfo info;
    while (true) {
      SDL_snprintf(ext, sizeof(ext), ".%d.bmp", num);
      imp_save_path(path, "Screenshots", ext);
      if (!SDL_GetPathInfo(path, &info)) {
        break;
      }
      ++num;
    }
    SDL_SaveBMP(screenshot, path);
    SDL_DestroySurface(screenshot);
  }
}

static void imp_upscale()
{
  if (imp.scale < 8) {
    ++imp.scale;
    imp.dirty_config = true;
  }
  switch (imp.scale) {
    case 1: imp_popup("Scale x1", 2); break;
    case 2: imp_popup("Scale x2", 2); break;
    case 3: imp_popup("Scale x3", 2); break;
    case 4: imp_popup("Scale x4", 2); break;
    case 5: imp_popup("Scale x5", 2); break;
    case 6: imp_popup("Scale x6", 2); break;
    case 7: imp_popup("Scale x7", 2); break;
    case 8: imp_popup("Scale x8", 2); break;
  }
  imp_try_resize();
}

static void imp_downscale()
{
  if (imp.scale > 1) {
    --imp.scale;
    imp.dirty_config = true;
  }
  switch (imp.scale) {
    case 1: imp_popup("Scale x1", 2); break;
    case 2: imp_popup("Scale x2", 2); break;
    case 3: imp_popup("Scale x3", 2); break;
    case 4: imp_popup("Scale x4", 2); break;
    case 5: imp_popup("Scale x5", 2); break;
    case 6: imp_popup("Scale x6", 2); break;
    case 7: imp_popup("Scale x7", 2); break;
    case 8: imp_popup("Scale x8", 2); break;
  }
  imp_try_resize();
}

static void imp_toggle_pause()
{
  if (imp.help) {
    imp.help = false;
    imp.pause = false;
  } else {
    imp.pause = !imp.pause;
    if (imp.pause) {
      imp_popup("Pause", 2);
    }
  }
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

static void imp_toggle_fps()
{
  imp.fps = !imp.fps;
  imp.dirty_config = true;
}

static void imp_toggle_fps_test()
{
  imp.fps_test = !imp.fps_test;
  SDL_SetRenderVSync(imp.renderer, !imp.fps_test);
  imp_popup(imp.fps_test ? "Max FPS ON" : "Max FPS OFF", 2);
}

static void imp_slot(uint slot)
{
  imp.slot = slot;
  imp_popup(imp_save_slot_labels[imp.slot], 2);
}

static void imp_slot_next()
{
  imp.slot = (imp.slot + 1) % 10;
  imp_popup(imp_save_slot_labels[imp.slot], 2);
}

static void imp_joy1_dpad_up_down()
{
  imp.joy1_mask |= MN_INPUT_UP;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_DOWN) | MN_INPUT_UP;
}

static void imp_joy1_dpad_up_up()
{
  imp.joy1_mask &= ~MN_INPUT_UP;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_UP) | (imp.joy1_mask & MN_INPUT_DOWN);
}

static void imp_joy1_dpad_left_down()
{
  imp.joy1_mask |= MN_INPUT_LEFT;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT;
}

static void imp_joy1_dpad_left_up()
{
  imp.joy1_mask &= ~MN_INPUT_LEFT;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_LEFT) | (imp.joy1_mask & MN_INPUT_RIGHT);
}

static void imp_joy1_dpad_down_down()
{
  imp.joy1_mask |= MN_INPUT_DOWN;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_UP) | MN_INPUT_DOWN;
}

static void imp_joy1_dpad_down_up()
{
  imp.joy1_mask &= ~MN_INPUT_DOWN;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_DOWN) | (imp.joy1_mask & MN_INPUT_UP);
}

static void imp_joy1_dpad_right_down()
{
  imp.joy1_mask |= MN_INPUT_RIGHT;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT;
}

static void imp_joy1_dpad_right_up()
{
  imp.joy1_mask &= ~MN_INPUT_RIGHT;
  imp.joy1 = (imp.joy1 & ~MN_INPUT_RIGHT) | (imp.joy1_mask & MN_INPUT_LEFT);
}

static void imp_joy1_a_down() { imp.joy1 |= MN_INPUT_A; }
static void imp_joy1_a_up() { imp.joy1 &= ~MN_INPUT_A; }
static void imp_joy1_b_down() { imp.joy1 |= MN_INPUT_B; }
static void imp_joy1_b_up() { imp.joy1 &= ~MN_INPUT_B; }
static void imp_joy1_a_turbo_down() { imp.joy1_turbo_a = true; }
static void imp_joy1_a_turbo_up() { imp.joy1_turbo_a = false; }
static void imp_joy1_b_turbo_down() { imp.joy1_turbo_b = true; }
static void imp_joy1_b_turbo_up() { imp.joy1_turbo_b = false; }
static void imp_joy1_select_down() { imp.joy1 |= MN_INPUT_SELECT; }
static void imp_joy1_select_up() { imp.joy1 &= ~MN_INPUT_SELECT; }
static void imp_joy1_start_down() { imp.joy1 |= MN_INPUT_START; }
static void imp_joy1_start_up() { imp.joy1 &= ~MN_INPUT_START; }

static void imp_joy2_dpad_up_down()
{
  imp.joy2_mask |= MN_INPUT_UP;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_DOWN) | MN_INPUT_UP;
}

static void imp_joy2_dpad_up_up()
{
  imp.joy2_mask &= ~MN_INPUT_UP;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_UP) | (imp.joy2_mask & MN_INPUT_DOWN);
}

static void imp_joy2_dpad_left_down()
{
  imp.joy2_mask |= MN_INPUT_LEFT;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_RIGHT) | MN_INPUT_LEFT;
}

static void imp_joy2_dpad_left_up()
{
  imp.joy2_mask &= ~MN_INPUT_LEFT;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_LEFT) | (imp.joy2_mask & MN_INPUT_RIGHT);
}

static void imp_joy2_dpad_down_down()
{
  imp.joy2_mask |= MN_INPUT_DOWN;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_UP) | MN_INPUT_DOWN;
}

static void imp_joy2_dpad_down_up()
{
  imp.joy2_mask &= ~MN_INPUT_DOWN;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_DOWN) | (imp.joy2_mask & MN_INPUT_UP);
}

static void imp_joy2_dpad_right_down()
{
  imp.joy2_mask |= MN_INPUT_RIGHT;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_LEFT) | MN_INPUT_RIGHT;
}

static void imp_joy2_dpad_right_up()
{
  imp.joy2_mask &= ~MN_INPUT_RIGHT;
  imp.joy2 = (imp.joy2 & ~MN_INPUT_RIGHT) | (imp.joy2_mask & MN_INPUT_LEFT);
}

static void imp_joy2_a_down() { imp.joy2 |= MN_INPUT_A; }
static void imp_joy2_a_up() { imp.joy2 &= ~MN_INPUT_A; }
static void imp_joy2_b_down() { imp.joy2 |= MN_INPUT_B; }
static void imp_joy2_b_up() { imp.joy2 &= ~MN_INPUT_B; }
static void imp_joy2_a_turbo_down() { imp.joy2_turbo_a = true; }
static void imp_joy2_a_turbo_up() { imp.joy2_turbo_a = false; }
static void imp_joy2_b_turbo_down() { imp.joy2_turbo_b = true; }
static void imp_joy2_b_turbo_up() { imp.joy2_turbo_b = false; }

static void imp_drop_file(const char* path)
{
  if (path) {
    imp_rom_load(path);
    imp_rom_update();
  }
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
  switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: imp_onresize(); break;
    case SDL_EVENT_DROP_FILE: imp_drop_file(event->drop.data); break;
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
        case SDLK_F1: imp_help(); break;
        case SDLK_F2: imp_open_file(); break;
        case SDLK_F3: imp_toggle_region(); break;
        case SDLK_F4: mn_reset(); break;
        case SDLK_F5: imp_save(); break;
        case SDLK_F6: imp_toggle_aspect(); break;
        case SDLK_F7: imp_toggle_overscan(); break;
        case SDLK_F8: imp_load(); break;
        case SDLK_F10: return SDL_APP_SUCCESS;
        case SDLK_F11: imp_toggle_fullscreen(); break;
        case SDLK_F12: imp_screenshot(); break;
        case SDLK_EQUALS: imp_upscale(); break;
        case SDLK_MINUS: imp_downscale(); break;
        case SDLK_ESCAPE: imp_toggle_pause(); break;
        case SDLK_TAB: imp_fast_forward_on(); break;
        case SDLK_GRAVE: imp_toggle_fps(); break;
        case SDLK_Z: imp_toggle_fps_test(); break;
        case SDLK_0:
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        case SDLK_9: imp_slot(event->key.key - SDLK_0); break;

        case SDLK_W: imp_joy1_dpad_up_down(); break;
        case SDLK_A: imp_joy1_dpad_left_down(); break;
        case SDLK_S: imp_joy1_dpad_down_down(); break;
        case SDLK_D: imp_joy1_dpad_right_down(); break;
        case SDLK_J: imp_joy1_b_down(); break;
        case SDLK_K: imp_joy1_a_down(); break;
        case SDLK_U: imp_joy1_b_turbo_down(); break;
        case SDLK_I: imp_joy1_a_turbo_down(); break;
        case SDLK_SPACE:
        case SDLK_F: imp_joy1_select_down(); break;
        case SDLK_RETURN:
        case SDLK_H: imp_joy1_start_down(); break;

        case SDLK_UP: imp_joy2_dpad_up_down(); break;
        case SDLK_LEFT: imp_joy2_dpad_left_down(); break;
        case SDLK_DOWN: imp_joy2_dpad_down_down(); break;
        case SDLK_RIGHT: imp_joy2_dpad_right_down(); break;
        case SDLK_KP_1: imp_joy2_b_down(); break;
        case SDLK_KP_2: imp_joy2_a_down(); break;
        case SDLK_KP_4: imp_joy2_b_turbo_down(); break;
        case SDLK_KP_5: imp_joy2_a_turbo_down(); break;
      }
      break;
    case SDL_EVENT_KEY_UP:
      switch (event->key.key) {
        case SDLK_TAB: imp_fast_forward_off(); break;

        case SDLK_W: imp_joy1_dpad_up_up(); break;
        case SDLK_A: imp_joy1_dpad_left_up(); break;
        case SDLK_S: imp_joy1_dpad_down_up(); break;
        case SDLK_D: imp_joy1_dpad_right_up(); break;
        case SDLK_J: imp_joy1_b_up(); break;
        case SDLK_K: imp_joy1_a_up(); break;
        case SDLK_U: imp_joy1_b_turbo_up(); break;
        case SDLK_I: imp_joy1_a_turbo_up(); break;
        case SDLK_SPACE:
        case SDLK_F: imp_joy1_select_up(); break;
        case SDLK_RETURN:
        case SDLK_H: imp_joy1_start_up(); break;

        case SDLK_UP: imp_joy2_dpad_up_up(); break;
        case SDLK_LEFT: imp_joy2_dpad_left_up(); break;
        case SDLK_DOWN: imp_joy2_dpad_down_up(); break;
        case SDLK_RIGHT: imp_joy2_dpad_right_up(); break;
        case SDLK_KP_1: imp_joy2_b_up(); break;
        case SDLK_KP_2: imp_joy2_a_up(); break;
        case SDLK_KP_4: imp_joy2_b_turbo_up(); break;
        case SDLK_KP_5: imp_joy2_a_turbo_up(); break;
      }
      break;
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
      switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: imp_slot_next(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: imp_fast_forward_on(); break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: imp_save(); break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: imp_load(); break;
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
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: imp_fast_forward_off(); break;
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
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*)
{
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
    if (++imp.turbo & 1) {
      if (imp.joy1_turbo_a) {
        imp.joy1 ^= MN_INPUT_A;
      }
      if (imp.joy1_turbo_b) {
        imp.joy1 ^= MN_INPUT_B;
      }
      if (imp.joy2_turbo_a) {
        imp.joy2 ^= MN_INPUT_A;
      }
      if (imp.joy2_turbo_b) {
        imp.joy2 ^= MN_INPUT_B;
      }
    }

    mn_frame(imp.joy1, imp.joy2);
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
    imp_save_config();
  }

  SDL_SetRenderScale(imp.renderer, 1, 1);
  SDL_SetRenderDrawColor(imp.renderer, 0, 0, 0, 0xFF);
  SDL_RenderClear(imp.renderer);

  SDL_UpdateTexture(imp.tex_out, nullptr, mn_video_data(), 256 * 4);
  SDL_RenderTexture(imp.renderer, imp.tex_out, nullptr, &imp.rect);

  SDL_SetRenderScale(imp.renderer, imp.ui_offset.w, imp.ui_offset.h);
  if (imp.help) {
    imp_draw_help();
  }
  if (imp.popup_time > 0) {
    imp_draw_popup();
  }
  if (imp.fps) {
    SDL_SetRenderDrawColor(imp.renderer, 0x80, 0, 0x80, 0xFF);
    SDL_RenderDebugTextFormat(imp.renderer, imp.ui_offset.x + 256 - 4 * 8, imp.ui_offset.y + 13, "%4d",
                              (int)imp.fps_value);
    SDL_SetRenderDrawColor(imp.renderer, 0x80, 0xFF, 0x80, 0xFF);
    SDL_RenderDebugTextFormat(imp.renderer, imp.ui_offset.x + 256 - 4 * 8 - 1, imp.ui_offset.y + 12, "%4d",
                              (int)imp.fps_value);
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
  SDL_DestroyTexture(imp.tex_out);
  SDL_DestroyTexture(imp.tex_font);
  SDL_DestroyRenderer(imp.renderer);
  SDL_DestroyWindow(imp.window);
}

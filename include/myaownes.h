#pragma once

typedef unsigned int uint;
typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long long i64;
typedef signed short i16;
typedef signed char i8;
typedef struct mn_rom* mn_rom;

enum
{
  MN_INPUT_A = (1 << 0),
  MN_INPUT_B = (1 << 1),
  MN_INPUT_SELECT = (1 << 2),
  MN_INPUT_START = (1 << 3),
  MN_INPUT_UP = (1 << 4),
  MN_INPUT_DOWN = (1 << 5),
  MN_INPUT_LEFT = (1 << 6),
  MN_INPUT_RIGHT = (1 << 7),
  MN_INPUT_TURBO_A = (1 << 8),
  MN_INPUT_TURBO_B = (1 << 9),
};

enum
{
  MN_REGION_NTSC = 0,
  MN_REGION_PAL,
  MN_REGION_AUTO,
  MN_REGION_DENDY,
};

struct mn_rom
{
  const u8* prg_rom;
  const u8* chr_rom;
  u8* prg_ram;
  u8* chr_ram;
  uint prg_rom_size;
  uint chr_rom_size;
  uint prg_ram_size;
  uint chr_ram_size;
  uint mapper;
  uint submapper;
  uint region;
  bool vert_mirror;
  bool prg_has_battery;
  bool chr_has_battery;
  bool has_trainer;
  bool alt_mirror;
  bool mapper_error;
};

mn_rom mn_rom_load(const char* rom_path, const char* sram_path);
bool mn_rom_save(const char* sram_path);
void mn_rom_release(mn_rom rom);
bool mn_rom_set(mn_rom rom);
mn_rom mn_rom_get();
void mn_region_set(uint region);
uint mn_region_get();
uint* mn_video_data();
void mn_video_size(bool auto_aspect, bool overscan, float scale, uint* dw, uint* dh);
void mn_video_fit(bool auto_aspect, bool overscan, float sw, float sh, float* dx, float* dy, float* dw, float* dh);
void mn_audio_freq(uint freq);
i16* mn_audio_data();
uint mn_audio_size();
void mn_reset();
void mn_frame(uint port1, uint port2);
bool mn_save(const char* path);
bool mn_load(const char* path);
void mn_palette(uint palette[64]);
void mn_apu_test_mode(bool enabled);

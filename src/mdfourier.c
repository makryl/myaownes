#include "myaownes.h"
#include <stdio.h>

typedef struct
{
  u8 magic[4];
  uint size;
  u8 format[4];

  u8 chunk1_id[4];
  uint chunk1_size;
  u16 audio_format;
  u16 channels;
  uint rate;
  uint brate;
  u16 align;
  u16 bps;

  u8 chunk2_id[4];
  uint chunk2_size;
} __attribute__((packed)) WavHeader;

int main(int, char**)
{
  const uint region = MN_REGION_NTSC;
  const char* rom_path = "../../external/nes-test-roms/Interactive/240pee/mdfourier4k.nes";
  const char* out_path = region == MN_REGION_PAL ? "../../tmp/myaownes_mdfourier_pal.wav"
                                                 : "../../tmp/myaownes_mdfourier.wav";
  uint seconds = region == MN_REGION_PAL ? 134 : 112;
  uint rate = 96000;
  uint channels = 1;
  uint sample_size = sizeof(i16);

  FILE* f = fopen(out_path, "wb");
  if (!f) {
    return 1;
  }

  uint data_size = seconds * rate * sample_size;

  WavHeader header = { .magic = { 'R', 'I', 'F', 'F' },
                       .size = 36 + data_size,
                       .format = { 'W', 'A', 'V', 'E' },

                       .chunk1_id = { 'f', 'm', 't', ' ' },
                       .chunk1_size = 16,
                       .audio_format = 1,
                       .channels = channels,
                       .rate = rate,
                       .brate = rate * channels * sample_size,
                       .align = channels * sample_size,
                       .bps = 8 * channels * sample_size,

                       .chunk2_id = { 'd', 'a', 't', 'a' },
                       .chunk2_size = data_size };

  fwrite(&header, sizeof(WavHeader), 1, f);

  mn_region_set(region);
  mn_audio_freq(rate);

  mn_rom rom = mn_rom_load(rom_path, nullptr);
  if (!rom) {
    return 1;
  }
  mn_rom_set(rom);
  uint total_size = 0;
  uint press_button_frame = 120;
  for (uint i = 0;; ++i) {
    uint joy1 = 0;
    if (i == press_button_frame) {
      joy1 = MN_INPUT_A;
    }

    mn_frame(joy1, 0);

    if (i < press_button_frame) {
      continue;
    }

    {
      i16* data = (i16*)mn_audio_data();
      uint count = mn_audio_size() / sizeof(i16);

      for (uint s = 0; s < count; s++) {
        data[s] = -data[s];
      }
    }

    uint write_size = mn_audio_size();
    if (total_size + write_size > data_size) {
      write_size = data_size - total_size;
    }
    fwrite(mn_audio_data(), 1, write_size, f);
    total_size += write_size;
    if (total_size >= data_size) {
      break;
    }
  }
  mn_rom_release(rom);
  fclose(f);

  return 0;
}

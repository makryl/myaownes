#include "myaownes.h"
#include "common.h"
#include <errno.h>

enum
{
  TAS_COMMAND_RESET = (1 << 0),
  TAS_COMMAND_POWER = (1 << 1),
};

void mn_tas_release(mn_tas tas) { free(tas); }
void mn_tas_reset(mn_tas tas) { tas->curr = 0; }

mn_tas mn_tas_load(const char* path)
{
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open %s: %s\n", path, strerror(errno));
    return nullptr;
  }

  uint frames = 0;

  char line[1024];
  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '|') {
      ++frames;
    }
  }
  fseek(f, 0, SEEK_SET);

  uint size = sizeof(struct mn_tas) + sizeof(uint) * 3 * frames;
  u8* mem = malloc(size);
  if (!mem) {
    errorf("no memory (%u) for %s", size, path);
    fclose(f);
    return nullptr;
  }

  mn_tas tas = (mn_tas)mem;
  tas->data = (uint*)(mem + sizeof(struct mn_tas));
  tas->size = 3 * frames;
  tas->curr = 0;

  uint* pdata = tas->data;

  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '|') {
      uint command = 0;
      uint port1 = 0;
      uint port2 = 0;
      char sport1[9] = {};
      char sport2[9] = {};
      if (sscanf(line, "|%d|%8[^|]|%8[^|]||", &command, sport1, sport2) != 3) {
        sscanf(line, "|%d|%8[^|]|||", &command, sport1);
      }
      for (uint i = 0; i < 8; ++i) {
        if (sport1[i] != 0 && sport1[i] != ' ' && sport1[i] != '.') {
          port1 |= 0x80 >> i;
        }
        if (sport2[i] != 0 && sport2[i] != ' ' && sport2[i] != '.') {
          port2 |= 0x80 >> i;
        }
      }
      *pdata = 0;
      if (command & 0x01) {
        *pdata |= TAS_COMMAND_RESET;
      }
      if (command & 0x02) {
        *pdata |= TAS_COMMAND_POWER;
      }
      ++pdata;
      *pdata = port1;
      ++pdata;
      *pdata = port2;
      ++pdata;
    }
  }
  fclose(f);

  return tas;
}

void mn_tas_frame(mn_tas tas)
{
  uint command = 0;
  uint port1 = 0;
  uint port2 = 0;
  if (tas->curr < tas->size - 2) {
    command = tas->data[tas->curr++];
    port1 = tas->data[tas->curr++];
    port2 = tas->data[tas->curr++];
  }
  if (command & 0x01) {
    mn_reset();
  }
  if (command & 0x02) {
    mn_power();
  }
  mn_frame(port1, port2);
}

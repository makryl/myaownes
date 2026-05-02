#include "myanes.h"

#include "common.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

bool read_file(const char* path, void** data, int* size)
{
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open file %s: %s\n", path, strerror(errno));
    return false;
  }

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);

  *data = malloc(*size);
  if (!*data) {
    errorf("not enough memory (%d) to read file %s\n", *size, path);
    fclose(f);
    return false;
  }

  if ((int)fread(*data, 1, *size, f) != *size) {
    errorf("error reading file %s: %s\n", path, strerror(errno));
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

int main(int, const char**)
{
  void* data;
  int size;
  if (!read_file("../../data/nestest.nes", &data, &size)) {
    return 1;
  }
  if (!mn_load(data, size)) {
    return 1;
  }
  mn_reset();
  for (int i = 0; i < 8991; ++i) {
    mn_tick();
  };
  free(data);
  return 0;
}

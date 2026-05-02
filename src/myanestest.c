#include "myanes.h"

int main(int, const char**)
{
  if (!mn_load("../../data/nestest.nes")) {
    return 1;
  }
  mn_reset();
  for (int i = 0; i < 8991; ++i) {
    mn_tick();
  };
  return 0;
}

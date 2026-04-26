#include "myanes.h"

int main(int, const char**)
{
  if (!mn_load("../../data/nestest.nes")) {
    return 1;
  }
  for (int i = 0; i < 1000; ++i) {
    mn_tick();
  };
  return 0;
}

#pragma once

#include "myaownes.h"

void dev_power();
void dev_region_update();
uint dev_bus_read(uint addr, uint val, bool trace);
void dev_bus_write(uint addr, uint val);
void dev_tick();

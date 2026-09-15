#pragma once

#include "myaownes.h"

void iou_power();
void iou_input(uint port1, uint port2);
uint iou_bus_read(uint addr, uint val, bool trace);
void iou_bus_write(uint addr, uint val);
void iou_tick();
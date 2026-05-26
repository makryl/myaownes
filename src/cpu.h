#pragma once

#include "myanes.h"

void cpu_power();
void cpu_reset();
void cpu_nmi(bool enabled);
void cpu_irq();
void cpu_tick();
u32 cpu_cyc();

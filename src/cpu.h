#pragma once

#include "myanes.h"

void cpu_power();
void cpu_reset();
void cpu_nmi(bool enabled);
void cpu_irq(bool enabled);
void cpu_dmc(u16 addr);
void cpu_tick();
u32 cpu_cyc();

#pragma once

#include "myaownes.h"

u32 cpu_size();
void* cpu_data();
void cpu_power();
void cpu_reset();
void cpu_nmi(bool enabled);
void cpu_irq(bool enabled);
void cpu_dmc(u16 addr);
void cpu_tick();
u32 cpu_cyc();

#pragma once

#include "myaownes.h"

uint cpu_size();
void* cpu_data();
void cpu_power();
void cpu_reset();
void cpu_nmi(bool enabled);
void cpu_irq(bool enabled);
void cpu_dmc(bool enabled, uint addr);
void cpu_tick();
uint cpu_cyc();

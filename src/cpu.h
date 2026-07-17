#pragma once

#include "myaownes.h"

uint cpu_size();
void* cpu_data();
void cpu_power();
void cpu_reset();
void cpu_input(u8 joy1, u8 joy2);
void cpu_nmi(bool enabled);
void cpu_irq(bool enabled);
void cpu_dmc(u16 addr);
void cpu_tick();
uint cpu_cyc();

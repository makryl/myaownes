#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "map.h"
#include "common.h"
#include <string.h>

enum
{
  CPU_ITR_RESET = 0xFFFC,
  CPU_ITR_NMI = 0xFFFA,
  CPU_ITR_IRQBRK = 0xFFFE,
};

enum
{
  CPU_FLAG_CARRY = (1 << 0),
  CPU_FLAG_ZERO = (1 << 1),
  CPU_FLAG_IRQ_DISABLED = (1 << 2),
  CPU_FLAG_DECIMAL = (1 << 3),
  CPU_FLAG_BREAK = (1 << 4),
  CPU_FLAG_ALWAYS_ONE = (1 << 5),
  CPU_FLAG_OVERFLOW = (1 << 6),
  CPU_FLAG_NEGATIVE = (1 << 7),
};

enum
{
  CPU_ADDR_IMP = 0,
  CPU_ADDR_IMM,
  CPU_ADDR_ACC,
  CPU_ADDR_REL,
  CPU_ADDR_IND,
  CPU_ADDR_ABS,
  CPU_ADDR_ABY,
  CPU_ADDR_ABX,
  CPU_ADDR_NDX,
  CPU_ADDR_NDY,
  CPU_ADDR_ZPG,
  CPU_ADDR_ZPX,
  CPU_ADDR_ZPY,
};

MN_CACHE_LINE static struct Cpu
{
  uint cyc;
  uint joy_shift_idx;
  uint joy_shift_delay;
  uint dmc_dma_delay;
  uint dmc_dma_addr;
  uint oam_dma_addr;
  uint itr;
  uint addr;
  uint pc;
  uint s;
  uint p;
  uint a;
  uint x;
  uint y;
  uint joy_pending[2];
  uint joy_shift[2];
  uint internal_open_bus;
  uint external_open_bus;
  uint out;

  bool reset: 1;
  bool nmi: 1;
  bool irq: 1;
  bool brk: 1;
  bool dmc_dma_halt: 1;
  bool dmc_dma_wait: 1;
  bool dmc_dma_done: 1;
  bool oam_dma_trig: 1;
  bool oam_dma_active: 1;
  bool page_crossed: 1;
  bool write: 1;
  bool suppress_poll: 1;
  bool pal: 1;

  MN_CACHE_LINE u8 ram[0x0800];
} cpu;

#define cpu_trace(fmt, ...)                                                                                            \
  tracef("CPU pc=%04X s=%02X p=%02X a=%02X x=%02X y=%02X sl=%-3d dot=%-3d cyc=%-10d  " fmt "\n", cpu.pc, cpu.s, cpu.p, \
         cpu.a, cpu.x, cpu.y, ppu_sl(), ppu_dot(), cpu.cyc __VA_OPT__(, ) __VA_ARGS__)

uint cpu_size() { return sizeof(cpu); }
void* cpu_data() { return &cpu; }

uint cpu_cyc() { return cpu.cyc; }

void cpu_reset() { cpu.reset = true; }
void cpu_nmi(bool enabled) { cpu.nmi = enabled; }
void cpu_irq(bool enabled) { cpu.irq = enabled; }

static void cpu_poll()
{
  if (cpu.suppress_poll) {
    return;
  }
  cpu.itr = 0;
  if (cpu.reset) {
    cpu.itr = CPU_ITR_RESET;
  } else if (cpu.nmi) {
    cpu.itr = CPU_ITR_NMI;
  } else if (cpu.brk || (cpu.irq && !(cpu.p & CPU_FLAG_IRQ_DISABLED))) {
    cpu.itr = CPU_ITR_IRQBRK;
  }
}

void cpu_power()
{
  memset(&cpu, 0, sizeof(cpu));
  cpu.pal = (mn_region_get() == MN_REGION_PAL);
  cpu_reset();
  cpu_poll();
}

void cpu_dmc(bool enabled, uint addr)
{
  if (enabled) {
    cpu.dmc_dma_addr = addr;
  }
  cpu.dmc_dma_halt = enabled;
  cpu.dmc_dma_wait = enabled;
}

static void cpu_dmc_dma();
static void cpu_oam_dma();

void cpu_input(uint joy1, uint joy2)
{
  cpu.joy_pending[0] = joy1;
  cpu.joy_pending[1] = joy2;
}

static uint cpu_joy_poll(uint addr, bool trace)
{
  uint idx = (addr & 1);
  if (!trace) {
    cpu.joy_shift_idx = idx;
    cpu.joy_shift_delay = 2;
  }
  uint val = (cpu.joy_shift[idx] & 1);
  // val |= (val & 1) << 1; // todo: Dendy uses expansion port for joy2?
  return val | (cpu.external_open_bus & 0xE0);
}

static void cpu_joy_strobe()
{
  if (cpu.joy_shift_delay > 0) {
    if (--cpu.joy_shift_delay == 0) {
      cpu.joy_shift[cpu.joy_shift_idx] = (cpu.joy_shift[cpu.joy_shift_idx] >> 1) | 0x80;
    }
  }
  if ((cpu.cyc & 1) && (cpu.out & 1)) {
    cpu.joy_shift[0] = cpu.joy_pending[0];
    cpu.joy_shift[1] = cpu.joy_pending[1];
  }
}

static void cpu_cyc_begin()
{
  cpu_oam_dma();
  if (!cpu.pal || cpu.addr == ((cpu.pc - 1) & 0xFFFF)) {
    cpu_dmc_dma();
  }
  cpu_poll();
  ppu_tick();
  ppu_tick();
  cpu_joy_strobe();
  apu_tick();
  map_cpu_cyc();
}

static void cpu_cyc_end()
{
  ppu_tick();
  if (cpu.pal && (cpu.cyc % 5) == 4) {
    ppu_tick();
  }
  ++cpu.cyc;
}

static uint cpu_read_addr_raw(uint addr, bool trace)
{
  if (addr < 0x2000) {
    return cpu.ram[addr & 0x07FF];
  } else if (addr < 0x4000) {
    return ppu_bus_read(addr, trace);
  } else if (addr < 0x4020) {
    if (addr == 0x4016 || addr == 0x4017) {
      return cpu_joy_poll(addr, trace);
    } else {
      return apu_bus_read(addr, addr == 0x4015 ? cpu.internal_open_bus : cpu.external_open_bus, trace);
    }
  } else {
    return map_cpu_read(addr, cpu.external_open_bus, trace);
  }
}

static uint cpu_read_addr_direct(uint addr)
{
  cpu_cyc_begin();
  if ((addr & 0xFFE0) != 0x4000) {
    cpu.external_open_bus = cpu_read_addr_raw(addr, false);
  }
  if ((cpu.addr & 0xFFE0) == 0x4000) { // NES bug: internal addr activation by bus addr (DMA dont change it)
    uint internal_addr = (0x4000 | (addr & 0x1F));
    cpu.internal_open_bus = cpu_read_addr_raw(internal_addr, false);
    if (internal_addr == 0x4016 || internal_addr == 0x4017) { // NES bug: io bus conflict
      cpu.external_open_bus = (cpu.external_open_bus & 0xE0) | (cpu.internal_open_bus & 0x1F);
    }
  } else {
    cpu.internal_open_bus = cpu.external_open_bus;
  }
  cpu_cyc_end();
  return cpu.internal_open_bus;
}

static uint cpu_read_addr(uint addr)
{
  cpu.addr = addr;
  return cpu_read_addr_direct(addr);
}

static void cpu_write_addr_direct(uint addr, uint val)
{
  cpu.write = true;
  cpu.internal_open_bus = val;
  cpu.external_open_bus = val;
  cpu_cyc_begin();
  if (addr < 0x2000) {
    cpu.ram[addr & 0x07FF] = val;
  } else if (addr < 0x4000) {
    ppu_bus_write(addr, val);
  } else if (addr < 0x4020) {
    if (addr == 0x4014) {
      cpu.oam_dma_addr = val << 8;
      cpu.oam_dma_trig = true;
    } else if (addr == 0x4016) {
      cpu.out = val;
    } else {
      apu_bus_write(addr, val);
    }
  } else {
    map_cpu_write(addr, val);
  }
  cpu_cyc_end();
  cpu.write = false;
}

static void cpu_write_addr(uint addr, uint val)
{
  cpu.addr = addr;
  cpu_write_addr_direct(addr, val);
}

#if MN_TRACE_CPU
static struct
{
  struct Cpu cpu;
  const char* opname;
  int cpu_cyc;
  int ppu_dot;
  int ppu_sl;
  uint addr0;
  uint addr;
  uint val;
} trace;

#define trace_addr0(code) trace.addr0 = code
#define trace_addr(code) trace.addr = code, trace.val = cpu_read_addr_raw(trace.addr, true), trace.addr
#define trace_tick(fmt, ...)                                                                                       \
  tracef(fmt "  A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%d\n" __VA_OPT__(, ) __VA_ARGS__, trace.cpu.a, \
         trace.cpu.x, trace.cpu.y, trace.cpu.p, trace.cpu.s, trace.ppu_sl, trace.ppu_dot, trace.cpu_cyc)
#define trace_tick0(fmt, ...) \
  trace_tick("%04X  %02X       %4s " fmt, trace.cpu.pc, pc0, trace.opname __VA_OPT__(, ) __VA_ARGS__);
#define trace_tick1(fmt, ...) \
  trace_tick("%04X  %02X %02X    %4s " fmt, trace.cpu.pc, pc0, pc1, trace.opname __VA_OPT__(, ) __VA_ARGS__);
#define trace_tick2(fmt, ...) \
  trace_tick("%04X  %02X %02X %02X %4s " fmt, trace.cpu.pc, pc0, pc1, pc2, trace.opname __VA_OPT__(, ) __VA_ARGS__);

static void trace_tick_addr(uint am)
{
  uint pc0 = cpu_read_addr_raw(trace.cpu.pc + 0, true);
  uint pc1 = cpu_read_addr_raw(trace.cpu.pc + 1, true);
  uint pc2 = cpu_read_addr_raw(trace.cpu.pc + 2, true);
  switch (am) {
    case CPU_ADDR_IMP: trace_tick0("                          "); break;
    case CPU_ADDR_ACC: trace_tick0("A                         "); break;
    case CPU_ADDR_IMM: trace_tick1("#$%02X                      ", pc1); break;
    case CPU_ADDR_REL: trace_tick1("$%04X                     ", trace.addr); break;
    case CPU_ADDR_ABS: {
      uint op = cpu_read_addr_raw(trace.cpu.pc, true);
      if (op == 0x20 || op == 0x4C) {
        trace_tick2("$%04X                     ", trace.addr);
      } else {
        trace_tick2("$%04X = %02X                ", trace.addr, trace.val);
      }
      break;
    }
    case CPU_ADDR_ABX: trace_tick2("$%02X%02X,X @ %04X = %02X       ", pc2, pc1, trace.addr, trace.val); break;
    case CPU_ADDR_ABY: trace_tick2("$%02X%02X,Y @ %04X = %02X       ", pc2, pc1, trace.addr, trace.val); break;
    case CPU_ADDR_IND: trace_tick2("($%02X%02X) = %04X            ", pc2, pc1, trace.addr); break;
    case CPU_ADDR_NDX: trace_tick1("($%02X,X) @ %02X = %04X = %02X  ", pc1, trace.addr0, trace.addr, trace.val); break;
    case CPU_ADDR_NDY: trace_tick1("($%02X),Y = %04X @ %04X = %02X", pc1, trace.addr0, trace.addr, trace.val); break;
    case CPU_ADDR_ZPG: trace_tick1("$%02X = %02X                  ", pc1, trace.val); break;
    case CPU_ADDR_ZPX: trace_tick1("$%02X,X @ %02X = %02X           ", pc1, trace.addr, trace.val); break;
    case CPU_ADDR_ZPY: trace_tick1("$%02X,Y @ %02X = %02X           ", pc1, trace.addr, trace.val); break;
  }
}

#define trace_opcode(val, op, am) \
  trace.opname = val;             \
  op(am);                         \
  trace_tick_addr(am)
#else
#define trace_addr0(code) code
#define trace_addr(code) code
#define trace_opcode(val, op, am) op(am)
#endif


static uint cpu_flag_zn(uint val)
{
  if (val == 0) {
    cpu.p |= CPU_FLAG_ZERO;
  } else {
    cpu.p &= ~CPU_FLAG_ZERO;
  }
  if (val & 0x80) {
    cpu.p |= CPU_FLAG_NEGATIVE;
  } else {
    cpu.p &= ~CPU_FLAG_NEGATIVE;
  }
  return val;
}

static void cpu_flag_carry(bool cond)
{
  if (cond) {
    cpu.p |= CPU_FLAG_CARRY;
  } else {
    cpu.p &= ~CPU_FLAG_CARRY;
  }
}

static void cpu_flag_overflow(bool cond)
{
  if (cond) {
    cpu.p |= CPU_FLAG_OVERFLOW;
  } else {
    cpu.p &= ~CPU_FLAG_OVERFLOW;
  }
}

static uint cpu_read16_addr(uint addr)
{
  uint lo = cpu_read_addr(addr);
  uint hi = cpu_read_addr((addr + 1) & 0xFFFF) << 8;
  return lo | hi;
}

static uint cpu_read16_zptr(uint zptr)
{
  uint lo = cpu_read_addr(zptr);
  uint hi = cpu_read_addr((zptr + 1) & 0xFF) << 8;
  return lo | hi;
}

static uint cpu_pc_inc()
{
  uint addr = cpu.pc;
  cpu.pc = ((cpu.pc + 1) & 0xFFFF);
  return addr;
}

static uint cpu_read_pc() { return cpu_read_addr(cpu_pc_inc()); }

static uint cpu_read16_pc()
{
  uint lo = cpu_read_pc();
  uint hi = cpu_read_pc() << 8;
  return lo | hi;
}

static uint cpu_stack_addr() { return (0x0100 | cpu.s); }

static uint cpu_stack_addr_dec()
{
  uint val = cpu_stack_addr();
  cpu.s = ((cpu.s - 1) & 0xFF);
  return val;
}

static void cpu_stack_push(uint val) { cpu_write_addr(cpu_stack_addr_dec(), val); }

static uint cpu_stack_pop(bool seq)
{
  if (!seq) {
    cpu_read_addr(cpu_stack_addr());
  }
  cpu.s = ((cpu.s + 1) & 0xFF);
  return cpu_read_addr(cpu_stack_addr());
}

static void cpu_stack_push16(uint val)
{
  cpu_stack_push((val >> 8) & 0xFF);
  cpu_stack_push(val & 0xFF);
}

static uint cpu_stack_pop16(bool seq)
{
  uint lo = cpu_stack_pop(seq);
  uint hi = cpu_stack_pop(true) << 8;
  return lo | hi;
}

static uint cpu_addr_offset(uint addr, int offset, bool readonly)
{
  uint target_addr = ((addr + offset) & 0xFFFF);
  cpu.page_crossed = (addr & 0xFF00) != (target_addr & 0xFF00);
  if (cpu.page_crossed || !readonly) {
    cpu_read_addr((addr & 0xFF00) | (target_addr & 0xFF));
  }
  return target_addr;
}

static uint cpu_addr(uint am, bool readonly)
{
  cpu.page_crossed = false;
  switch (am) {
    case CPU_ADDR_ACC:
    case CPU_ADDR_IMP: return cpu.pc;
    case CPU_ADDR_IMM: return cpu_pc_inc();
    case CPU_ADDR_IND: {
      uint addr0 = trace_addr0(cpu_read16_pc());
      uint lo = cpu_read_addr(addr0);
      uint hi_addr = (addr0 & 0xFF00) | ((addr0 + 1) & 0x00FF); // NES bug: for hi byte read, inc only low addr byte
      uint hi = cpu_read_addr(hi_addr) << 8;
      return trace_addr(lo | hi);
    }
    case CPU_ADDR_REL: {
      int off = (int)(i8)cpu_read_pc();
      cpu.suppress_poll = true; // NES bug: cyc for offset without poll
      cpu_read_addr(cpu.pc);
      cpu.suppress_poll = false;
      return trace_addr(cpu_addr_offset(cpu.pc, off, readonly));
    }
    case CPU_ADDR_ABS: return trace_addr(cpu_read16_pc());
    case CPU_ADDR_ABX: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_pc()), cpu.x, readonly));
    case CPU_ADDR_ABY: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_pc()), cpu.y, readonly));
    case CPU_ADDR_NDX: {
      uint zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr = ((zptr + cpu.x) & 0xFF);
      return trace_addr(cpu_read16_zptr(trace_addr0(zptr)));
    }
    case CPU_ADDR_NDY: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_zptr(cpu_read_pc())), cpu.y, readonly));
    case CPU_ADDR_ZPG: return trace_addr(cpu_read_pc());
    case CPU_ADDR_ZPX: {
      uint zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr = ((zptr + cpu.x) & 0xFF);
      return trace_addr(zptr);
    }
    case CPU_ADDR_ZPY: {
      uint zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr = ((zptr + cpu.y) & 0xFF);
      return trace_addr(zptr);
    }
  }
  return 0;
}

static uint cpu_read(uint am) { return cpu_read_addr(cpu_addr(am, true)); }
static void cpu_write(uint am, uint val) { cpu_write_addr(cpu_addr(am, false), val); }

static uint cpu_read_write(uint am, uint (*cb)(uint))
{
  uint addr = cpu_addr(am, false);
  uint val = cpu_read_addr(addr);
  if (am == CPU_ADDR_ACC) {
    cpu.a = cpu_flag_zn(cb(cpu.a));
    return cpu.a;
  }
  cpu_write_addr(addr, val);
  val = cb(val);
  cpu_write_addr(addr, val);
  return val;
}

static void cpu_op_LDA(uint am) { cpu.a = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_LDX(uint am) { cpu.x = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_LDY(uint am) { cpu.y = cpu_flag_zn(cpu_read(am)); }

static void cpu_op_STA(uint am) { cpu_write(am, cpu.a); }
static void cpu_op_STX(uint am) { cpu_write(am, cpu.x); }
static void cpu_op_STY(uint am) { cpu_write(am, cpu.y); }

static void cpu_op_TAX(uint am) { cpu_read(am), cpu.x = cpu_flag_zn(cpu.a); }
static void cpu_op_TAY(uint am) { cpu_read(am), cpu.y = cpu_flag_zn(cpu.a); }
static void cpu_op_TXA(uint am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu.x); }
static void cpu_op_TYA(uint am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu.y); }

static void cpu_op_TSX(uint am) { cpu_read(am), cpu.x = cpu_flag_zn(cpu.s); }
static void cpu_op_TXS(uint am) { cpu_read(am), cpu.s = cpu.x; }

static void cpu_op_PHA(uint am) { cpu_read(am), cpu_stack_push(cpu.a); }
static void cpu_op_PHP(uint am) { cpu_read(am), cpu_stack_push(cpu.p | CPU_FLAG_BREAK | CPU_FLAG_ALWAYS_ONE); }
static void cpu_op_PLA(uint am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu_stack_pop(false)); }
static void cpu_op_PLP(uint am)
{
  cpu_read(am), cpu.p = (cpu_stack_pop(false) & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE;
}

static void cpu_and(uint val) { cpu.a = cpu_flag_zn(cpu.a & val); }
static void cpu_ora(uint val) { cpu.a = cpu_flag_zn(cpu.a | val); }
static void cpu_eor(uint val) { cpu.a = cpu_flag_zn(cpu.a ^ val); }

static void cpu_op_AND(uint am) { cpu_and(cpu_read(am)); }
static void cpu_op_ORA(uint am) { cpu_ora(cpu_read(am)); }
static void cpu_op_EOR(uint am) { cpu_eor(cpu_read(am)); }

static void cpu_adc(uint val)
{
  uint res = cpu.a + val + (cpu.p & CPU_FLAG_CARRY);
  cpu_flag_carry(res > 0xFF);
  res &= 0xFF;
  cpu_flag_overflow((~(cpu.a ^ val) & (cpu.a ^ res)) & 0x80);
  cpu.a = cpu_flag_zn(res);
}

static void cpu_sbc(uint val) { cpu_adc(~val & 0xFF); }

static void cpu_op_ADC(uint am) { cpu_adc(cpu_read(am)); }
static void cpu_op_SBC(uint am) { cpu_sbc(cpu_read(am)); }

static uint cpu_inc(uint val) { return cpu_flag_zn((val + 1) & 0xFF); }
static void cpu_op_INC(uint am) { cpu_read_write(am, cpu_inc); }
static void cpu_op_INX(uint am) { cpu_read(am), cpu_flag_zn(cpu.x = ((cpu.x + 1) & 0xFF)); }
static void cpu_op_INY(uint am) { cpu_read(am), cpu_flag_zn(cpu.y = ((cpu.y + 1) & 0xFF)); }

static uint cpu_dec(uint val) { return cpu_flag_zn((val - 1) & 0xFF); }
static void cpu_op_DEC(uint am) { cpu_read_write(am, cpu_dec); }
static void cpu_op_DEX(uint am) { cpu_read(am), cpu_flag_zn(cpu.x = ((cpu.x - 1) & 0xFF)); }
static void cpu_op_DEY(uint am) { cpu_read(am), cpu_flag_zn(cpu.y = ((cpu.y - 1) & 0xFF)); }

static uint cpu_asl(uint val)
{
  cpu_flag_carry(val & 0x80);
  return cpu_flag_zn((val << 1) & 0xFF);
}

static uint cpu_lsr(uint val)
{
  cpu_flag_carry(val & 1);
  return cpu_flag_zn(val >> 1);
}

static uint cpu_rol(uint val)
{
  uint carry = (cpu.p & CPU_FLAG_CARRY);
  cpu_flag_carry(val & 0x80);
  return cpu_flag_zn(((val << 1) & 0xFF) | carry);
}

static uint cpu_ror(uint val)
{
  uint carry = (cpu.p & CPU_FLAG_CARRY) ? 0x80 : 0;
  cpu_flag_carry(val & 1);
  return cpu_flag_zn((val >> 1) | carry);
}

static void cpu_op_ASL(uint am) { cpu_read_write(am, cpu_asl); }
static void cpu_op_LSR(uint am) { cpu_read_write(am, cpu_lsr); }
static void cpu_op_ROL(uint am) { cpu_read_write(am, cpu_rol); }
static void cpu_op_ROR(uint am) { cpu_read_write(am, cpu_ror); }

static void cpu_op_CLC(uint am) { cpu_read(am), cpu.p &= ~CPU_FLAG_CARRY; }
static void cpu_op_CLD(uint am) { cpu_read(am), cpu.p &= ~CPU_FLAG_DECIMAL; }
static void cpu_op_CLI(uint am) { cpu_read(am), cpu.p &= ~CPU_FLAG_IRQ_DISABLED; }
static void cpu_op_CLV(uint am) { cpu_read(am), cpu.p &= ~CPU_FLAG_OVERFLOW; }
static void cpu_op_SEC(uint am) { cpu_read(am), cpu.p |= CPU_FLAG_CARRY; }
static void cpu_op_SED(uint am) { cpu_read(am), cpu.p |= CPU_FLAG_DECIMAL; }
static void cpu_op_SEI(uint am) { cpu_read(am), cpu.p |= CPU_FLAG_IRQ_DISABLED; }

static uint cpu_cmp(uint val, uint reg)
{
  cpu_flag_carry(reg >= val);
  return cpu_flag_zn((reg - val) & 0xFF);
}

static void cpu_op_CMP(uint am) { cpu_cmp(cpu_read(am), cpu.a); }
static void cpu_op_CPX(uint am) { cpu_cmp(cpu_read(am), cpu.x); }
static void cpu_op_CPY(uint am) { cpu_cmp(cpu_read(am), cpu.y); }

static void cpu_op_BIT(uint am)
{
  uint val = cpu_read(am);
  if ((cpu.a & val) == 0) {
    cpu.p |= CPU_FLAG_ZERO;
  } else {
    cpu.p &= ~CPU_FLAG_ZERO;
  }
  cpu.p &= ~(CPU_FLAG_NEGATIVE | CPU_FLAG_OVERFLOW);
  cpu.p |= (val & 0xC0);
}

static void cpu_branch(uint am, uint flag, bool cond)
{
  if (((cpu.p & flag) != 0) == cond) {
    cpu.pc = cpu_addr(am, true);
  } else {
    int off = (int)(i8)cpu_read_pc();
    (void)(trace_addr((cpu.pc + off) & 0xFFFF));
  }
}

static void cpu_op_BPL(uint am) { cpu_branch(am, CPU_FLAG_NEGATIVE, 0); }
static void cpu_op_BMI(uint am) { cpu_branch(am, CPU_FLAG_NEGATIVE, 1); }
static void cpu_op_BVC(uint am) { cpu_branch(am, CPU_FLAG_OVERFLOW, 0); }
static void cpu_op_BVS(uint am) { cpu_branch(am, CPU_FLAG_OVERFLOW, 1); }
static void cpu_op_BCC(uint am) { cpu_branch(am, CPU_FLAG_CARRY, 0); }
static void cpu_op_BCS(uint am) { cpu_branch(am, CPU_FLAG_CARRY, 1); }
static void cpu_op_BNE(uint am) { cpu_branch(am, CPU_FLAG_ZERO, 0); }
static void cpu_op_BEQ(uint am) { cpu_branch(am, CPU_FLAG_ZERO, 1); }

static void cpu_op_JMP(uint am) { cpu.pc = cpu_addr(am, true); }

static void cpu_op_JSR(uint)
{
  uint lo = cpu_read_pc();
  cpu_read_addr(cpu_stack_addr());
  cpu_stack_push16(cpu.pc);
  uint hi = cpu_read_pc() << 8;
  trace_addr(cpu.pc = lo | hi);
}

static void cpu_op_RTS(uint am)
{
  cpu_read(am);
  cpu.pc = cpu_stack_pop16(false);
  cpu_read_addr(cpu_pc_inc());
}

static void cpu_op_BRK(uint am)
{
  cpu.brk = true;
  cpu_read(am);
}

static void cpu_op_RTI(uint am)
{
  cpu_read(am);
  cpu.p = (cpu_stack_pop(false) & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE;
  cpu.pc = cpu_stack_pop16(true);
}

static void cpu_op_NOP(uint am) { cpu_read(am); }

static void cpu_op_KIL(uint)
{
  cpu.pc = ((cpu.pc - 1) & 0xFFFF);
  errorf("KIL $%02X\n", cpu_read_addr_raw(cpu.pc, true));
}

static void cpu_op_SLO(uint am) { cpu_ora(cpu_read_write(am, cpu_asl)); }
static void cpu_op_RLA(uint am) { cpu_and(cpu_read_write(am, cpu_rol)); }
static void cpu_op_SRE(uint am) { cpu_eor(cpu_read_write(am, cpu_lsr)); }
static void cpu_op_RRA(uint am) { cpu_adc(cpu_read_write(am, cpu_ror)); }
static void cpu_op_SAX(uint am) { cpu_write(am, cpu.a & cpu.x); }
static void cpu_op_LAX(uint am) { cpu.a = cpu.x = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_DCP(uint am) { cpu_cmp(cpu_read_write(am, cpu_dec), cpu.a); }
static void cpu_op_ISB(uint am) { cpu_sbc(cpu_read_write(am, cpu_inc)); }

static void cpu_op_LAR(uint am) { cpu.a = cpu.x = cpu.s = cpu_flag_zn(cpu_read(am) & cpu.s); }
static void cpu_op_AAC(uint am) { cpu_op_AND(am), cpu_flag_carry(cpu.a & 0x80); }
static void cpu_op_ASR(uint am) { cpu_op_AND(am), cpu.a = cpu_lsr(cpu.a); }
static void cpu_op_AXS(uint am) { cpu.x = cpu_cmp(cpu_read(am), cpu.a & cpu.x); }
static void cpu_op_ATX(uint am) { cpu_op_LDA(am), cpu.x = cpu.a; }
static void cpu_op_XAA(uint am) { cpu.a = cpu_flag_zn(cpu_read(am) & cpu.x); }

static void cpu_op_ARR(uint am)
{
  cpu_op_AND(am);
  cpu.a = cpu_ror(cpu.a);
  cpu_flag_carry(cpu.a & 0x40);
  cpu_flag_overflow(((cpu.a & 0x40) >> 6) ^ ((cpu.a & 0x20) >> 5));
}

static void cpu_sh(uint am, uint val)
{
  uint addr = cpu_addr(am, false);
  uint hi = addr >> 8;
  if (cpu.page_crossed) {
    addr &= ((hi & val) << 8) | 0xFF;
  }
  if (!cpu.dmc_dma_done) {
    val &= cpu.page_crossed ? hi : ((hi + 1) & 0xFF);
  }
  cpu_write_addr(addr, val);
}

static void cpu_op_SHA(uint am) { cpu_sh(am, cpu.x & cpu.a); }
static void cpu_op_SHS(uint am) { cpu_sh(am, cpu.s = (cpu.x & cpu.a)); }
static void cpu_op_SHX(uint am) { cpu_sh(am, cpu.x); }
static void cpu_op_SHY(uint am) { cpu_sh(am, cpu.y); }

static void cpu_itr_exec()
{
  uint addr = cpu.itr;

  if (!cpu.brk) {
    cpu_read_addr(cpu.pc);
    cpu_read_addr(cpu.pc);
  }

  if (addr == CPU_ITR_RESET) {
    cpu_read_addr(cpu_stack_addr_dec());
    cpu_read_addr(cpu_stack_addr_dec());
    cpu_read_addr(cpu_stack_addr_dec());
  } else {
    cpu_stack_push16(cpu.pc);
    cpu_stack_push(cpu.p | (cpu.brk ? CPU_FLAG_BREAK : 0) | CPU_FLAG_ALWAYS_ONE);
  }

  if (cpu.itr) {
    addr = cpu.itr;
  }

  cpu.pc = cpu_read16_addr(addr);
  cpu.p |= CPU_FLAG_IRQ_DISABLED | CPU_FLAG_ALWAYS_ONE;

  if (addr == CPU_ITR_NMI) {
    cpu.nmi = false;
  }
  cpu.reset = false;
  cpu.brk = false;

#if MN_TRACE_NESTEST
  if (addr == CPU_ITR_RESET) {
    cpu.pc = 0xC000;
  }
#endif
}

static void cpu_dma_dummy() { cpu_read_addr_direct(cpu.addr); }

static void cpu_dma_align()
{
  if (cpu.cyc & 1) {
    cpu_dma_dummy();
  }
}


static void cpu_dmc_read()
{
  cpu.dmc_dma_wait = false;
  uint val = cpu_read_addr_direct(cpu.dmc_dma_addr);
  apu_dmc_dma(val); // will trace wrong incremented cpu_cyc
}

static void cpu_dmc_dma()
{
  cpu.dmc_dma_done = false;
  if (cpu.oam_dma_active) {
    if (cpu.dmc_dma_halt) {
      cpu.dmc_dma_delay = (cpu_cyc() & 1) ? 3 : 2; // halt/dummy/align can overlap with oam dma
      cpu.dmc_dma_halt = false;
    } else if (cpu.dmc_dma_delay > 0) {
      if (--cpu.dmc_dma_delay == 0 && cpu.dmc_dma_wait) {
        cpu_dmc_read();
        if (!cpu.write) {
          cpu_dma_dummy(); // align
        }
      }
    }
  } else {
    while (!cpu.write && cpu.dmc_dma_halt) {
      cpu.dmc_dma_halt = false;
      cpu_dma_dummy(); // halt
      if (cpu.dmc_dma_wait) {
        cpu_dma_dummy(); // dummy
        cpu_dma_align();
        cpu_dmc_read();
      }
      cpu.dmc_dma_done = true;
    }
  }
}

static void cpu_oam_dma()
{
  if (!cpu.oam_dma_trig || cpu.write) { // todo: skip dummy write in read-modify-write or every write?
    return;
  }
  cpu.oam_dma_trig = false;
  cpu.oam_dma_active = true;
  cpu_dma_dummy(); // halt
  cpu_dma_align();
  for (uint i = 0; i < 256; ++i) {
    uint val = cpu_read_addr_direct(cpu.oam_dma_addr | i);
    cpu_write_addr_direct(0x2004, val);
  }
  if (cpu.dmc_dma_delay > 0) {
    while (cpu.dmc_dma_delay > 1) {
      cpu_dma_dummy(); // halt/dummy/align tail
    }
    cpu.dmc_dma_delay = 0;
    cpu_dmc_read();
  }
  cpu.oam_dma_active = false;
}

void cpu_tick()
{
  if (cpu.itr) {
    cpu_itr_exec();
  }

#if MN_TRACE_CPU
  trace.cpu = cpu;
  trace.cpu_cyc = cpu.cyc;
  trace.ppu_dot = ppu_dot();
  trace.ppu_sl = ppu_sl();
#endif

  switch (cpu_read_pc()) {
    // clang-format off
#define L(cc, aaa, o0, o1, o2, o3, o4, o5, o6, o7) \
  case (aaa << 5) | (0 << 2) | cc: o0; break; \
  case (aaa << 5) | (2 << 2) | cc: o1; break; \
  case (aaa << 5) | (4 << 2) | cc: o2; break; \
  case (aaa << 5) | (6 << 2) | cc: o3; break; \
  case (aaa << 5) | (1 << 2) | cc: o4; break; \
  case (aaa << 5) | (3 << 2) | cc: o5; break; \
  case (aaa << 5) | (5 << 2) | cc: o6; break; \
  case (aaa << 5) | (7 << 2) | cc: o7; break;
#define O(op, addr) trace_opcode(#op, cpu_op_##op, CPU_ADDR_##addr)
#define X(op, addr) trace_opcode("*" #op, cpu_op_##op, CPU_ADDR_##addr)
/*        xxx000xx     xxx010xx     xxx100xx     xxx110xx     xxx001xx     xxx011xx     xxx101xx     xxx111xx */
L(0, 0, O(BRK, IMM), O(PHP, IMP), O(BPL, REL), O(CLC, IMP), X(NOP, ZPG), X(NOP, ABS), X(NOP, ZPX), X(NOP, ABX))
L(0, 1, O(JSR, ABS), O(PLP, IMP), O(BMI, REL), O(SEC, IMP), O(BIT, ZPG), O(BIT, ABS), X(NOP, ZPX), X(NOP, ABX))
L(0, 2, O(RTI, IMP), O(PHA, IMP), O(BVC, REL), O(CLI, IMP), X(NOP, ZPG), O(JMP, ABS), X(NOP, ZPX), X(NOP, ABX))
L(0, 3, O(RTS, IMP), O(PLA, IMP), O(BVS, REL), O(SEI, IMP), X(NOP, ZPG), O(JMP, IND), X(NOP, ZPX), X(NOP, ABX))
L(0, 4, X(NOP, IMM), O(DEY, IMP), O(BCC, REL), O(TYA, IMP), O(STY, ZPG), O(STY, ABS), O(STY, ZPX), X(SHY, ABX))
L(0, 5, O(LDY, IMM), O(TAY, IMP), O(BCS, REL), O(CLV, IMP), O(LDY, ZPG), O(LDY, ABS), O(LDY, ZPX), O(LDY, ABX))
L(0, 6, O(CPY, IMM), O(INY, IMP), O(BNE, REL), O(CLD, IMP), O(CPY, ZPG), O(CPY, ABS), X(NOP, ZPX), X(NOP, ABX))
L(0, 7, O(CPX, IMM), O(INX, IMP), O(BEQ, REL), O(SED, IMP), O(CPX, ZPG), O(CPX, ABS), X(NOP, ZPX), X(NOP, ABX))
L(1, 0, O(ORA, NDX), O(ORA, IMM), O(ORA, NDY), O(ORA, ABY), O(ORA, ZPG), O(ORA, ABS), O(ORA, ZPX), O(ORA, ABX))
L(1, 1, O(AND, NDX), O(AND, IMM), O(AND, NDY), O(AND, ABY), O(AND, ZPG), O(AND, ABS), O(AND, ZPX), O(AND, ABX))
L(1, 2, O(EOR, NDX), O(EOR, IMM), O(EOR, NDY), O(EOR, ABY), O(EOR, ZPG), O(EOR, ABS), O(EOR, ZPX), O(EOR, ABX))
L(1, 3, O(ADC, NDX), O(ADC, IMM), O(ADC, NDY), O(ADC, ABY), O(ADC, ZPG), O(ADC, ABS), O(ADC, ZPX), O(ADC, ABX))
L(1, 4, O(STA, NDX), X(NOP, IMM), O(STA, NDY), O(STA, ABY), O(STA, ZPG), O(STA, ABS), O(STA, ZPX), O(STA, ABX))
L(1, 5, O(LDA, NDX), O(LDA, IMM), O(LDA, NDY), O(LDA, ABY), O(LDA, ZPG), O(LDA, ABS), O(LDA, ZPX), O(LDA, ABX))
L(1, 6, O(CMP, NDX), O(CMP, IMM), O(CMP, NDY), O(CMP, ABY), O(CMP, ZPG), O(CMP, ABS), O(CMP, ZPX), O(CMP, ABX))
L(1, 7, O(SBC, NDX), O(SBC, IMM), O(SBC, NDY), O(SBC, ABY), O(SBC, ZPG), O(SBC, ABS), O(SBC, ZPX), O(SBC, ABX))
L(2, 0, X(KIL, IMP), O(ASL, ACC), X(KIL, IMP), X(NOP, IMP), O(ASL, ZPG), O(ASL, ABS), O(ASL, ZPX), O(ASL, ABX))
L(2, 1, X(KIL, IMP), O(ROL, ACC), X(KIL, IMP), X(NOP, IMP), O(ROL, ZPG), O(ROL, ABS), O(ROL, ZPX), O(ROL, ABX))
L(2, 2, X(KIL, IMP), O(LSR, ACC), X(KIL, IMP), X(NOP, IMP), O(LSR, ZPG), O(LSR, ABS), O(LSR, ZPX), O(LSR, ABX))
L(2, 3, X(KIL, IMP), O(ROR, ACC), X(KIL, IMP), X(NOP, IMP), O(ROR, ZPG), O(ROR, ABS), O(ROR, ZPX), O(ROR, ABX))
L(2, 4, X(NOP, IMM), O(TXA, IMP), X(KIL, IMP), O(TXS, IMP), O(STX, ZPG), O(STX, ABS), O(STX, ZPY), X(SHX, ABY))
L(2, 5, O(LDX, IMM), O(TAX, IMP), X(KIL, IMP), O(TSX, IMP), O(LDX, ZPG), O(LDX, ABS), O(LDX, ZPY), O(LDX, ABY))
L(2, 6, X(NOP, IMM), O(DEX, IMP), X(KIL, IMP), X(NOP, IMP), O(DEC, ZPG), O(DEC, ABS), O(DEC, ZPX), O(DEC, ABX))
L(2, 7, X(NOP, IMM), O(NOP, IMP), X(KIL, IMP), X(NOP, IMP), O(INC, ZPG), O(INC, ABS), O(INC, ZPX), O(INC, ABX))
L(3, 0, X(SLO, NDX), X(AAC, IMM), X(SLO, NDY), X(SLO, ABY), X(SLO, ZPG), X(SLO, ABS), X(SLO, ZPX), X(SLO, ABX))
L(3, 1, X(RLA, NDX), X(AAC, IMM), X(RLA, NDY), X(RLA, ABY), X(RLA, ZPG), X(RLA, ABS), X(RLA, ZPX), X(RLA, ABX))
L(3, 2, X(SRE, NDX), X(ASR, IMM), X(SRE, NDY), X(SRE, ABY), X(SRE, ZPG), X(SRE, ABS), X(SRE, ZPX), X(SRE, ABX))
L(3, 3, X(RRA, NDX), X(ARR, IMM), X(RRA, NDY), X(RRA, ABY), X(RRA, ZPG), X(RRA, ABS), X(RRA, ZPX), X(RRA, ABX))
L(3, 4, X(SAX, NDX), X(XAA, IMM), X(SHA, NDY), X(SHS, ABY), X(SAX, ZPG), X(SAX, ABS), X(SAX, ZPY), X(SHA, ABY))
L(3, 5, X(LAX, NDX), X(ATX, IMM), X(LAX, NDY), X(LAR, ABY), X(LAX, ZPG), X(LAX, ABS), X(LAX, ZPY), X(LAX, ABY))
L(3, 6, X(DCP, NDX), X(AXS, IMM), X(DCP, NDY), X(DCP, ABY), X(DCP, ZPG), X(DCP, ABS), X(DCP, ZPX), X(DCP, ABX))
L(3, 7, X(ISB, NDX), X(SBC, IMM), X(ISB, NDY), X(ISB, ABY), X(ISB, ZPG), X(ISB, ABS), X(ISB, ZPX), X(ISB, ABX))
#undef L
#undef O
#undef X
    // clang-format on
  }

#if MN_TRACE_NESTEST
  if (cpu.cyc > 26554) {
    fflush(stdout);
    __builtin_trap();
  }
#endif
}

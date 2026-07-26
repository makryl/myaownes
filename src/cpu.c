#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "map.h"
#include "common.h"
#include <string.h>

enum : u16
{
  CPU_ITR_RESET = 0xFFFC,
  CPU_ITR_NMI = 0xFFFA,
  CPU_ITR_IRQBRK = 0xFFFE,
};

enum : u8
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
  uint joy_idx;
  uint joy_shift_delay;
  uint dmc_dma_delay;
  u16 oam_dma_addr;
  u16 dmc_dma_addr;
  u16 itr;
  u16 addr;
  u16 pc;
  u8 s;
  u8 p;
  u8 a;
  u8 x;
  u8 y;
  u8 joy_pending[2];
  u8 joy_shift[2];
  u8 open_bus;
  bool reset;
  bool nmi;
  bool irq;
  bool brk;
  bool oam_dma;
  bool dmc_dma;
  bool joy_strobe;
  bool page_crossed;
  bool write;
  bool suppress_poll;
  bool pal;

  MN_CACHE_LINE u8 ram[0x0800];
} cpu;

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
  mn_rom rom = mn_rom_get();
  cpu.pal = (rom->tv == MN_TV_PAL);
  cpu_reset();
  cpu_poll();
}

void cpu_dmc(u16 addr)
{
  cpu.dmc_dma_addr = addr;
  cpu.dmc_dma = true;
}

static void cpu_dmc_dma();

void cpu_input(u8 joy1, u8 joy2)
{
  cpu.joy_pending[0] = joy1;
  cpu.joy_pending[1] = joy2;
}

static u8 cpu_joy_poll(u16 addr)
{
  u8 val;
  cpu.joy_idx = (addr & 1);
  if (cpu.joy_strobe) {
    val = (cpu.joy_pending[cpu.joy_idx] & 1);
  } else {
    cpu.joy_shift_delay = 2;
    val = (cpu.joy_shift[cpu.joy_idx] & 1);
  }
  val |= (val & 1) << 1;
  return val | (cpu.open_bus & 0xE0);
}

static void cpu_joy_shift()
{
  if (cpu.joy_shift_delay > 0) {
    if (--cpu.joy_shift_delay == 0) {
      cpu.joy_shift[cpu.joy_idx] = (cpu.joy_shift[cpu.joy_idx] >> 1) | 0x80;
    }
  }
}

static void cpu_joy_strobe(u8 val)
{
  cpu.joy_strobe = (val & 1);
  if (cpu.joy_strobe) {
    cpu.joy_shift[0] = cpu.joy_pending[0];
    cpu.joy_shift[1] = cpu.joy_pending[1];
  }
}

static void cpu_cyc_begin()
{
  if (!cpu.pal || cpu.addr == cpu.pc - 1) {
    cpu_dmc_dma();
  }
  cpu_poll();
  ppu_tick();
  ppu_tick();
  apu_tick();
  map_cpu_cyc();
}

static void cpu_cyc_end()
{
  cpu_joy_shift();
  ppu_tick();
  if (cpu.pal && (cpu.cyc % 5) == 4) {
    ppu_tick();
  }
  ++cpu.cyc;
}

static u8 cpu_read_addr_(u16 addr, bool trace)
{
  u8 val = cpu.open_bus;
  if (addr < 0x2000) {
    val = cpu.ram[addr & 0x07FF];
  } else if (addr < 0x4000) {
    val = ppu_bus_read(addr, trace);
  } else if (addr < 0x4020) {
    if (addr == 0x4016 || addr == 0x4017) {
      val = cpu_joy_poll(addr);
    } else {
      apu_bus_read(addr, &val, trace);
    }
  } else {
    map_cpu_read(addr, &val, trace);
  }
  return val;
}

static u8 cpu_read_addr(u16 addr)
{
  cpu.addr = addr;
  cpu_cyc_begin();
  cpu.open_bus = cpu_read_addr_(addr, false);
  cpu_cyc_end();
  return cpu.open_bus;
}

static void cpu_write_addr(u16 addr, u8 val)
{
  cpu.write = true;
  cpu.addr = addr;
  cpu.open_bus = val;
  cpu_cyc_begin();
  if (addr < 0x2000) {
    cpu.ram[addr & 0x07FF] = val;
  } else if (addr < 0x4000) {
    ppu_bus_write(addr, val);
  } else if (addr < 0x4020) {
    if (addr == 0x4014) {
      cpu.oam_dma_addr = val << 8;
      cpu.oam_dma = true;
    } else if (addr == 0x4016) {
      cpu_joy_strobe(val);
    } else {
      apu_bus_write(addr, val);
    }
  } else {
    map_cpu_write(addr, val);
  }
  cpu_cyc_end();
  cpu.write = false;
}

#if MN_TRACE_CPU
static struct
{
  struct Cpu cpu;
  const char* opname;
  int cpu_cyc;
  int ppu_dot;
  int ppu_sl;
  u16 addr0;
  u16 addr;
  u8 val;
} trace;

#define trace_addr0(code) trace.addr0 = code
#define trace_addr(code) trace.addr = code, trace.val = cpu_read_addr_(trace.addr, true), trace.addr
#define trace_tick(fmt)                                                                                              \
  _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-extra-args\"")                       \
    tracef(fmt "  A:%6$02X X:%7$02X Y:%8$02X P:%9$02X SP:%10$02X PPU:%11$3d,%12$3d CYC:%13$d\n",                     \
           cpu_read_addr_(trace.cpu.pc + 1, true), cpu_read_addr_(trace.cpu.pc + 2, true),                           \
           cpu_read_addr_(trace.cpu.pc, true), trace.cpu.pc, trace.opname, trace.cpu.a, trace.cpu.x, trace.cpu.y,    \
           trace.cpu.p, trace.cpu.s, trace.ppu_sl, trace.ppu_dot, trace.cpu_cyc, trace.addr0, trace.addr, trace.val) \
      _Pragma("clang diagnostic pop")
#define trace_tick0(fmt) trace_tick("%4$04X  %3$02X       %5$4s " fmt);
#define trace_tick1(fmt) trace_tick("%4$04X  %3$02X %1$02X    %5$4s " fmt);
#define trace_tick2(fmt) trace_tick("%4$04X  %3$02X %1$02X %2$02X %5$4s " fmt);

static void trace_tick_addr(u8 am)
{
  switch (am) {
    case CPU_ADDR_IMP: trace_tick0("                          "); break;
    case CPU_ADDR_ACC: trace_tick0("A                         "); break;
    case CPU_ADDR_IMM: trace_tick1("#$%1$02X                      "); break;
    case CPU_ADDR_REL: trace_tick1("$%15$04X                     "); break;
    case CPU_ADDR_ABS: {
      u8 op = cpu_read_addr_(trace.cpu.pc, true);
      if (op == 0x20 || op == 0x4C) {
        trace_tick2("$%15$04X                     ");
      } else {
        trace_tick2("$%15$04X = %16$02X                ");
      }
      break;
    }
    case CPU_ADDR_ABX: trace_tick2("$%2$02X%1$02X,X @ %15$04X = %16$02X       "); break;
    case CPU_ADDR_ABY: trace_tick2("$%2$02X%1$02X,Y @ %15$04X = %16$02X       "); break;
    case CPU_ADDR_IND: trace_tick2("($%2$02X%1$02X) = %15$04X            "); break;
    case CPU_ADDR_NDX: trace_tick1("($%1$02X,X) @ %14$02X = %15$04X = %16$02X  "); break;
    case CPU_ADDR_NDY: trace_tick1("($%1$02X),Y = %14$04X @ %15$04X = %16$02X"); break;
    case CPU_ADDR_ZPG: trace_tick1("$%1$02X = %16$02X                  "); break;
    case CPU_ADDR_ZPX: trace_tick1("$%1$02X,X @ %15$02X = %16$02X           "); break;
    case CPU_ADDR_ZPY: trace_tick1("$%1$02X,Y @ %15$02X = %16$02X           "); break;
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


static u8 cpu_flag_zn(u8 val)
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

static u16 cpu_read16_addr(u16 addr) { return (u16)cpu_read_addr(addr) | ((u16)cpu_read_addr(addr + 1) << 8); }
static u16 cpu_read16_zptr(u8 zptr) { return (u16)cpu_read_addr(zptr) | ((u16)cpu_read_addr((u8)(zptr + 1)) << 8); }
static u8 cpu_read_pc() { return cpu_read_addr(cpu.pc++); }
static u16 cpu_read16_pc() { return (u16)cpu_read_pc() | ((u16)cpu_read_pc() << 8); }

static void cpu_stack_push(u8 val) { cpu_write_addr(0x0100 | cpu.s--, val); }
static u8 cpu_stack_pop(bool seq)
{
  if (!seq) {
    cpu_read_addr(cpu.s | 0x0100);
  }
  return cpu_read_addr(++cpu.s | 0x0100);
}
static void cpu_stack_push16(u16 val) { cpu_stack_push((u8)(val >> 8)), cpu_stack_push((u8)(val & 0xFF)); }
static u16 cpu_stack_pop16(bool seq) { return (u16)cpu_stack_pop(seq) | ((u16)cpu_stack_pop(seq) << 8); }

static u16 cpu_addr_offset(u16 addr, int offset, bool readonly)
{
  u16 target_addr = addr + offset;
  cpu.page_crossed = (addr & 0xFF00) != (target_addr & 0xFF00);
  if (cpu.page_crossed || !readonly) {
    cpu_read_addr((addr & 0xFF00) | (target_addr & 0xFF));
  }
  return target_addr;
}

static u16 cpu_addr(u8 am, bool readonly)
{
  cpu.page_crossed = false;
  switch (am) {
    case CPU_ADDR_ACC:
    case CPU_ADDR_IMP: return cpu.pc;
    case CPU_ADDR_IMM: return cpu.pc++;
    case CPU_ADDR_IND: {
      u16 addr0 = trace_addr0(cpu_read16_pc());
      u8 lo = cpu_read_addr(addr0);
      u16 hi_addr = (addr0 & 0xFF00) | ((addr0 + 1) & 0x00FF); // NES bug: for hi byte read, inc only low addr byte
      u8 hi = cpu_read_addr(hi_addr);
      return trace_addr((u16)lo | ((u16)hi << 8));
    }
    case CPU_ADDR_REL: {
      i8 off = (i8)cpu_read_pc();
      cpu.suppress_poll = true; // NES bug: cyc for offset without poll
      cpu_read_addr(cpu.pc);
      cpu.suppress_poll = false;
      return trace_addr(cpu_addr_offset(cpu.pc, off, readonly));
    }
    case CPU_ADDR_ABS: return trace_addr(cpu_read16_pc());
    case CPU_ADDR_ABX: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_pc()), cpu.x, readonly));
    case CPU_ADDR_ABY: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_pc()), cpu.y, readonly));
    case CPU_ADDR_NDX: {
      u8 zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr += cpu.x;
      return trace_addr(cpu_read16_zptr(trace_addr0(zptr)));
    }
    case CPU_ADDR_NDY: return trace_addr(cpu_addr_offset(trace_addr0(cpu_read16_zptr(cpu_read_pc())), cpu.y, readonly));
    case CPU_ADDR_ZPG: return trace_addr(cpu_read_pc());
    case CPU_ADDR_ZPX: {
      u8 zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr += cpu.x;
      return trace_addr(zptr);
    }
    case CPU_ADDR_ZPY: {
      u8 zptr = cpu_read_pc();
      cpu_read_addr(zptr);
      zptr += cpu.y;
      return trace_addr(zptr);
    }
  }
  return 0;
}

static u8 cpu_read(u8 am) { return cpu_read_addr(cpu_addr(am, true)); }
static void cpu_write(u8 am, u8 val) { cpu_write_addr(cpu_addr(am, false), val); }

static u8 cpu_read_write(u8 am, u8 (*cb)(u8))
{
  u16 addr = cpu_addr(am, false);
  u8 val = cpu_read_addr(addr);
  if (am == CPU_ADDR_ACC) {
    cpu.a = cpu_flag_zn(cb(cpu.a));
    return cpu.a;
  }
  cpu_write_addr(addr, val);
  val = cb(val);
  cpu_write_addr(addr, val);
  return val;
}

static void cpu_op_LDA(u8 am) { cpu.a = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_LDX(u8 am) { cpu.x = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_LDY(u8 am) { cpu.y = cpu_flag_zn(cpu_read(am)); }

static void cpu_op_STA(u8 am) { cpu_write(am, cpu.a); }
static void cpu_op_STX(u8 am) { cpu_write(am, cpu.x); }
static void cpu_op_STY(u8 am) { cpu_write(am, cpu.y); }

static void cpu_op_TAX(u8 am) { cpu_read(am), cpu.x = cpu_flag_zn(cpu.a); }
static void cpu_op_TAY(u8 am) { cpu_read(am), cpu.y = cpu_flag_zn(cpu.a); }
static void cpu_op_TXA(u8 am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu.x); }
static void cpu_op_TYA(u8 am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu.y); }

static void cpu_op_TSX(u8 am) { cpu_read(am), cpu.x = cpu_flag_zn(cpu.s); }
static void cpu_op_TXS(u8 am) { cpu_read(am), cpu.s = cpu.x; }

static void cpu_op_PHA(u8 am) { cpu_read(am), cpu_stack_push(cpu.a); }
static void cpu_op_PHP(u8 am) { cpu_read(am), cpu_stack_push(cpu.p | CPU_FLAG_BREAK | CPU_FLAG_ALWAYS_ONE); }
static void cpu_op_PLA(u8 am) { cpu_read(am), cpu.a = cpu_flag_zn(cpu_stack_pop(false)); }
static void cpu_op_PLP(u8 am) { cpu_read(am), cpu.p = (cpu_stack_pop(false) & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE; }

static void cpu_and(u8 val) { cpu.a = cpu_flag_zn(cpu.a & val); }
static void cpu_ora(u8 val) { cpu.a = cpu_flag_zn(cpu.a | val); }
static void cpu_eor(u8 val) { cpu.a = cpu_flag_zn(cpu.a ^ val); }

static void cpu_op_AND(u8 am) { cpu_and(cpu_read(am)); }
static void cpu_op_ORA(u8 am) { cpu_ora(cpu_read(am)); }
static void cpu_op_EOR(u8 am) { cpu_eor(cpu_read(am)); }

static void cpu_adc(u8 val)
{
  uint res = (uint)cpu.a + (uint)val + (uint)(cpu.p & CPU_FLAG_CARRY);
  cpu_flag_carry(res > 0xFF);
  cpu_flag_overflow((~(cpu.a ^ val) & (cpu.a ^ (u8)res)) & 0x80);
  cpu.a = cpu_flag_zn((u8)res);
}

static void cpu_sbc(u8 val) { cpu_adc(~val); }

static void cpu_op_ADC(u8 am) { cpu_adc(cpu_read(am)); }
static void cpu_op_SBC(u8 am) { cpu_sbc(cpu_read(am)); }

static u8 cpu_inc(u8 val) { return cpu_flag_zn(val + 1); }
static void cpu_op_INC(u8 am) { cpu_read_write(am, cpu_inc); }
static void cpu_op_INX(u8 am) { cpu_read(am), cpu_flag_zn(++cpu.x); }
static void cpu_op_INY(u8 am) { cpu_read(am), cpu_flag_zn(++cpu.y); }

static u8 cpu_dec(u8 val) { return cpu_flag_zn(val - 1); }
static void cpu_op_DEC(u8 am) { cpu_read_write(am, cpu_dec); }
static void cpu_op_DEX(u8 am) { cpu_read(am), cpu_flag_zn(--cpu.x); }
static void cpu_op_DEY(u8 am) { cpu_read(am), cpu_flag_zn(--cpu.y); }

static u8 cpu_asl(u8 val)
{
  cpu_flag_carry(val & 0x80);
  return cpu_flag_zn(val << 1);
}

static u8 cpu_lsr(u8 val)
{
  cpu_flag_carry(val & 1);
  return cpu_flag_zn(val >> 1);
}

static u8 cpu_rol(u8 val)
{
  u8 carry = (cpu.p & CPU_FLAG_CARRY);
  cpu_flag_carry(val & 0x80);
  return cpu_flag_zn((val << 1) | carry);
}

static u8 cpu_ror(u8 val)
{
  u8 carry = (cpu.p & CPU_FLAG_CARRY) ? 0x80 : 0;
  cpu_flag_carry(val & 1);
  return cpu_flag_zn((val >> 1) | carry);
}

static void cpu_op_ASL(u8 am) { cpu_read_write(am, cpu_asl); }
static void cpu_op_LSR(u8 am) { cpu_read_write(am, cpu_lsr); }
static void cpu_op_ROL(u8 am) { cpu_read_write(am, cpu_rol); }
static void cpu_op_ROR(u8 am) { cpu_read_write(am, cpu_ror); }

static void cpu_op_CLC(u8 am) { cpu_read(am), cpu.p &= ~CPU_FLAG_CARRY; }
static void cpu_op_CLD(u8 am) { cpu_read(am), cpu.p &= ~CPU_FLAG_DECIMAL; }
static void cpu_op_CLI(u8 am) { cpu_read(am), cpu.p &= ~CPU_FLAG_IRQ_DISABLED; }
static void cpu_op_CLV(u8 am) { cpu_read(am), cpu.p &= ~CPU_FLAG_OVERFLOW; }
static void cpu_op_SEC(u8 am) { cpu_read(am), cpu.p |= CPU_FLAG_CARRY; }
static void cpu_op_SED(u8 am) { cpu_read(am), cpu.p |= CPU_FLAG_DECIMAL; }
static void cpu_op_SEI(u8 am) { cpu_read(am), cpu.p |= CPU_FLAG_IRQ_DISABLED; }

static u8 cpu_cmp(u8 val, u8 reg)
{
  cpu_flag_carry(reg >= val);
  return cpu_flag_zn(reg - val);
}

static void cpu_op_CMP(u8 am) { cpu_cmp(cpu_read(am), cpu.a); }
static void cpu_op_CPX(u8 am) { cpu_cmp(cpu_read(am), cpu.x); }
static void cpu_op_CPY(u8 am) { cpu_cmp(cpu_read(am), cpu.y); }

static void cpu_op_BIT(u8 am)
{
  u8 val = cpu_read(am);
  if ((cpu.a & val) == 0) {
    cpu.p |= CPU_FLAG_ZERO;
  } else {
    cpu.p &= ~CPU_FLAG_ZERO;
  }
  cpu.p &= ~(CPU_FLAG_NEGATIVE | CPU_FLAG_OVERFLOW);
  cpu.p |= (val & 0xC0);
}

static void cpu_branch(u8 am, u8 flag, bool cond)
{
  if (((cpu.p & flag) != 0) == cond) {
    cpu.pc = cpu_addr(am, true);
  } else {
    i8 off = (i8)cpu_read_pc();
    (void)(trace_addr(cpu.pc + off));
  }
}

static void cpu_op_BPL(u8 am) { cpu_branch(am, CPU_FLAG_NEGATIVE, 0); }
static void cpu_op_BMI(u8 am) { cpu_branch(am, CPU_FLAG_NEGATIVE, 1); }
static void cpu_op_BVC(u8 am) { cpu_branch(am, CPU_FLAG_OVERFLOW, 0); }
static void cpu_op_BVS(u8 am) { cpu_branch(am, CPU_FLAG_OVERFLOW, 1); }
static void cpu_op_BCC(u8 am) { cpu_branch(am, CPU_FLAG_CARRY, 0); }
static void cpu_op_BCS(u8 am) { cpu_branch(am, CPU_FLAG_CARRY, 1); }
static void cpu_op_BNE(u8 am) { cpu_branch(am, CPU_FLAG_ZERO, 0); }
static void cpu_op_BEQ(u8 am) { cpu_branch(am, CPU_FLAG_ZERO, 1); }

static void cpu_op_JMP(u8 am) { cpu.pc = cpu_addr(am, true); }

static void cpu_op_JSR(u8 am)
{
  u16 addr = cpu_addr(am, true); // read hi byte at end?
  cpu_read_addr(cpu.s | 0x0100); // implement inside of cpu_stack_push?
  cpu_stack_push16(cpu.pc - 1);
  cpu.pc = addr;
}

static void cpu_op_RTS(u8 am) { cpu_read(am), cpu.pc = cpu_stack_pop16(false) + 1; }

static void cpu_op_BRK(u8 am)
{
  cpu.brk = true;
  cpu_read(am);
}

static void cpu_op_RTI(u8 am)
{
  cpu_read(am);
  cpu.p = (cpu_stack_pop(false) & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE;
  cpu.pc = cpu_stack_pop16(true);
}

static void cpu_op_NOP(u8 am) { cpu_read(am); }

static void cpu_op_KIL(u8)
{
  --cpu.pc;
  errorf("KIL $%02X\n", cpu_read_addr_(cpu.pc, true));
}

static void cpu_op_SLO(u8 am) { cpu_ora(cpu_read_write(am, cpu_asl)); }
static void cpu_op_RLA(u8 am) { cpu_and(cpu_read_write(am, cpu_rol)); }
static void cpu_op_SRE(u8 am) { cpu_eor(cpu_read_write(am, cpu_lsr)); }
static void cpu_op_RRA(u8 am) { cpu_adc(cpu_read_write(am, cpu_ror)); }
static void cpu_op_SAX(u8 am) { cpu_write(am, cpu.a & cpu.x); }
static void cpu_op_LAX(u8 am) { cpu.a = cpu.x = cpu_flag_zn(cpu_read(am)); }
static void cpu_op_DCP(u8 am) { cpu_cmp(cpu_read_write(am, cpu_dec), cpu.a); }
static void cpu_op_ISB(u8 am) { cpu_sbc(cpu_read_write(am, cpu_inc)); }

static void cpu_op_LAR(u8 am) { cpu.a = cpu.x = cpu.s = cpu_flag_zn(cpu_read(am) & cpu.s); }
static void cpu_op_AAC(u8 am) { cpu_op_AND(am), cpu_flag_carry(cpu.a & 0x80); }
static void cpu_op_ASR(u8 am) { cpu_op_AND(am), cpu.a = cpu_lsr(cpu.a); }
static void cpu_op_AXS(u8 am) { cpu.x = cpu_cmp(cpu_read(am), cpu.a & cpu.x); }
static void cpu_op_ATX(u8 am) { cpu_op_LDA(am), cpu.x = cpu.a; } // unstable
static void cpu_op_XAA(u8 am) { cpu.a = cpu_flag_zn(cpu_read(am) & cpu.x); } // unstable

static void cpu_op_ARR(u8 am)
{
  cpu_op_AND(am);
  cpu.a = cpu_ror(cpu.a);
  cpu_flag_carry(cpu.a & 0x40);
  cpu_flag_overflow(((cpu.a & 0x40) >> 6) ^ ((cpu.a & 0x20) >> 5));
}

static void cpu_op_AXA(u8 am)
{
  u16 addr = cpu_addr(am, false);
  cpu_write_addr(addr, cpu.a & cpu.x & ((addr >> 8) + 1));
} // unstable

static void cpu_op_XAS(u8 am)
{
  u16 addr = cpu_addr(am, false);
  cpu.s = cpu.a & cpu.x;
  cpu_write_addr(addr, cpu.s & ((addr >> 8) + 1));
} // unstable

static void cpu_op_SXA(u8 am)
{
  u16 addr = cpu_addr(am, false);
  u8 op = addr >> 8;
  if (cpu.page_crossed) {
    u8 val = cpu.x & op;
    cpu_write_addr((val << 8) | (addr & 0xFF), val);
  } else {
    cpu_write_addr(addr, cpu.x & ++op);
  }
} // unstable

static void cpu_op_SYA(u8 am)
{
  u16 addr = cpu_addr(am, false);
  u8 op = addr >> 8;
  if (cpu.page_crossed) {
    u8 val = cpu.y & op;
    cpu_write_addr((val << 8) | (addr & 0xFF), val);
  } else {
    cpu_write_addr(addr, cpu.y & ++op);
  }
} // unstable

static void cpu_itr_exec()
{
  u16 addr = cpu.itr;

  if (!cpu.brk) {
    cpu_read_addr(cpu.pc);
    cpu_read_addr(cpu.pc);
  }

  if (addr == CPU_ITR_RESET) {
    cpu_read_addr(0x0100 | cpu.s--);
    cpu_read_addr(0x0100 | cpu.s--);
    cpu_read_addr(0x0100 | cpu.s--);
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

static void cpu_dma_align()
{
  if (cpu.cyc & 1) {
    cpu_read_addr(cpu.addr);
  }
}

static void cpu_dmc_read()
{
  cpu_cyc_begin(); // begin/end for correct trace cycles in apu_dmc_dma, also keep cpu.addr
  apu_dmc_dma(cpu_read_addr_(cpu.dmc_dma_addr, false));
  cpu_cyc_end();
}

static void cpu_dmc_dma()
{
  if (cpu.oam_dma) {
    if (cpu.dmc_dma) {
      cpu.dmc_dma_delay = (cpu_cyc() & 1) ? 3 : 2; // halt/dummy/align can overlap with oam dma
      cpu.dmc_dma = false;
    } else if (cpu.dmc_dma_delay > 0) {
      if (--cpu.dmc_dma_delay == 0) {
        cpu_dmc_read();
        if (!cpu.write) {
          cpu_read_addr(cpu.addr); // align
        }
      }
    }
  } else if (!cpu.write && cpu.dmc_dma) {
    cpu.dmc_dma = false;
    cpu_read_addr(cpu.addr); // halt
    cpu_read_addr(cpu.addr); // dummy
    cpu_dma_align();
    cpu_dmc_read();
  }
}

static void cpu_oam_dma()
{
  cpu_read_addr(cpu.addr); // halt
  cpu_dma_align();
  for (uint i = 0; i < 256; ++i) {
    u8 val = cpu_read_addr(cpu.oam_dma_addr | i);
    cpu_write_addr(0x2004, val);
  }
  if (cpu.dmc_dma_delay > 0) {
    while (cpu.dmc_dma_delay > 1) {
      cpu_read_addr(cpu.addr); // halt/dummy/align tail
    }
    cpu.dmc_dma_delay = 0;
    cpu_dmc_read();
  }
  cpu.oam_dma = false;
}

void cpu_tick()
{
  if (cpu.itr) {
    cpu_itr_exec();
  } else if (cpu.oam_dma) {
    cpu_oam_dma();
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
L(0, 4, X(NOP, IMM), O(DEY, IMP), O(BCC, REL), O(TYA, IMP), O(STY, ZPG), O(STY, ABS), O(STY, ZPX), X(SYA, ABX))
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
L(2, 4, X(NOP, IMM), O(TXA, IMP), X(KIL, IMP), O(TXS, IMP), O(STX, ZPG), O(STX, ABS), O(STX, ZPY), X(SXA, ABY))
L(2, 5, O(LDX, IMM), O(TAX, IMP), X(KIL, IMP), O(TSX, IMP), O(LDX, ZPG), O(LDX, ABS), O(LDX, ZPY), O(LDX, ABY))
L(2, 6, X(NOP, IMM), O(DEX, IMP), X(KIL, IMP), X(NOP, IMP), O(DEC, ZPG), O(DEC, ABS), O(DEC, ZPX), O(DEC, ABX))
L(2, 7, X(NOP, IMM), O(NOP, IMP), X(KIL, IMP), X(NOP, IMP), O(INC, ZPG), O(INC, ABS), O(INC, ZPX), O(INC, ABX))
L(3, 0, X(SLO, NDX), X(AAC, IMM), X(SLO, NDY), X(SLO, ABY), X(SLO, ZPG), X(SLO, ABS), X(SLO, ZPX), X(SLO, ABX))
L(3, 1, X(RLA, NDX), X(AAC, IMM), X(RLA, NDY), X(RLA, ABY), X(RLA, ZPG), X(RLA, ABS), X(RLA, ZPX), X(RLA, ABX))
L(3, 2, X(SRE, NDX), X(ASR, IMM), X(SRE, NDY), X(SRE, ABY), X(SRE, ZPG), X(SRE, ABS), X(SRE, ZPX), X(SRE, ABX))
L(3, 3, X(RRA, NDX), X(ARR, IMM), X(RRA, NDY), X(RRA, ABY), X(RRA, ZPG), X(RRA, ABS), X(RRA, ZPX), X(RRA, ABX))
L(3, 4, X(SAX, NDX), X(XAA, IMM), X(AXA, NDY), X(XAS, ABY), X(SAX, ZPG), X(SAX, ABS), X(SAX, ZPY), X(AXA, ABY))
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

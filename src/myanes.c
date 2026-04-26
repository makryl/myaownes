#include "myanes.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MN_TRACE
#define MN_TRACE 1
#endif

#ifndef MN_ERROR
#define MN_ERROR 1
#endif

#ifdef MN_TRACE
#define tracef(...) printf(__VA_ARGS__)
#else
#define tracef(...) ((void)0)
#endif

#ifdef MN_ERROR
#define errorf(...) fprintf(stderr, __VA_ARGS__)
#else
#define errorf(...) ((void)0)
#endif

typedef unsigned short u16;
typedef unsigned char u8;
typedef signed char i8;

enum NesFlag : u8
{
  NES_FLAG_MIRROR = (1 << 0),
  NES_FLAG_BATTERY = (1 << 1),
  NES_FLAG_TRAINER = (1 << 2),
  NES_FLAG_VRAM = (1 << 3),
};

enum CpuInterrupt : u16
{
  CPU_INT_RESET = 0xFFFC,
  CPU_INT_NMI = 0xFFFA,
  CPU_INT_IRQBRK = 0xFFFE,
};

enum CpuFlag : u8
{
  CPU_FLAG_CARRY = (1 << 0),
  CPU_FLAG_ZERO = (1 << 1),
  CPU_FLAG_INTERRUPT_DISABLED = (1 << 2),
  CPU_FLAG_DECIMAL = (1 << 3),
  CPU_FLAG_BREAK = (1 << 4),
  CPU_FLAG_ALWAYS_ONE = (1 << 5),
  CPU_FLAG_OVERFLOW = (1 << 6),
  CPU_FLAG_NEGATIVE = (1 << 7),
};

enum CpuAddr : u8
{
  CPU_ADDR_NDX = 0,
  CPU_ADDR_ZPG,
  CPU_ADDR_IMM,
  CPU_ADDR_ABS,
  CPU_ADDR_NDY,
  CPU_ADDR_ZPX,
  CPU_ADDR_ABY,
  CPU_ADDR_ABX,

  CPU_ADDR_ACC,
  CPU_ADDR_IMA,
  CPU_ADDR_STA,
  CPU_ADDR_ZPY,
  CPU_ADDR_XXX,
  CPU_ADDR_YYY,
};

static const size_t ROM_PAGE_SIZE = 0x4000;
static const size_t VROM_PAGE_SIZE = 0x2000;

static u8 ram[0x0800];
static u8 vram[0x0800];
static u8 wram[0x0800];
static u8 sram[0x2000];
static u8 eram[0x1000];

static u8 rom[0x400000]; // 256 * ROM_PAGE_SIZE
static u8 vrom[0x200000]; // 256 * VROM_PAGE_SIZE

static struct
{
  int cycle;
  int skip;
  int rom_pages;
  int vrom_pages;
  u8 flags;
  u8 mapper;
} ctx;

static struct CpuReg
{
  u8 a;
  u16 pc;
  u8 sp;
  u8 p;
  u8 x;
  u8 y;
} cpu_reg;

static struct
{
  u8 ctrl;
  u8 mask;
  u8 status;
  u8 oam_addr;
  u8 oam_data;
  u8 scroll;
  u8 addr;
  u8 data;
} ppu_reg;

static struct
{
  u8 apu[0x14];
  u8 oam_dma;
  u8 apu_ctrl_status;
  u8 joy1;
  u8 joy2_frame;
} apu_reg;

typedef struct
{
  u8 magic[4];
  u8 rom_pages;
  u8 vrom_pages;
  u8 flags;
  u8 mapper;
  u8 pad[8];
} NESHeader;

static u8* cpu_map_(u16 ptr)
{
  if (ptr < 0x2000) {
    return ram + (ptr & 0x07FF);
  }
  if (ptr < 0x4000) {
    return &ppu_reg.ctrl + (ptr & 0x7);
  }
  if (ptr < 0x4018) {
    return apu_reg.apu + (ptr & 0x1F);
  }
  if (ptr < 0x5000) {
    errorf("cpu_map failed 0x4018 < %04X < 0x5000\n", ptr);
    return 0; // todo
  }
  if (ptr < 0x6000) {
    return eram + (ptr & 0xFFF);
  }
  if (ptr < 0x8000) {
    return sram + (ptr & 0x1FFF);
  }
  if (ctx.rom_pages == 1) {
    return rom + (ptr & 0x3FFF);
  }
  return rom + (ptr & 0x7FFF);
}

static u8* cpu_map(u16 ptr)
{
  ++ctx.cycle;
  return cpu_map_(ptr);
}

static void* ppu_map(u16)
{
  //
  return 0;
}


#ifdef MN_TRACE
static int start_cycle = 0;
static struct CpuReg start_cpu_reg;
static const char* trace_opname = "";

#define trace_op(val) trace_opname = val
#define trace_opcode(val, code) \
  trace_opname = val;           \
  code
#define trace_tick(fmt, ...)                                                                                   \
  _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-extra-args\"")                 \
    tracef(fmt "  A:%6$02X X:%7$02X Y:%8$02X P:%9$02X SP:%10$02X PPU:%11$3d,%12$3d CYC:%13$d\n",               \
           *cpu_map_(start_cpu_reg.pc + 1), *cpu_map_(start_cpu_reg.pc + 2), *cpu_map_(start_cpu_reg.pc),      \
           start_cpu_reg.pc, trace_opname, start_cpu_reg.a, start_cpu_reg.x, start_cpu_reg.y, start_cpu_reg.p, \
           start_cpu_reg.sp, -10, -10, start_cycle, __VA_ARGS__) _Pragma("clang diagnostic pop")

#else
#define trace_op(val) (void)0
#define trace_opcode(val, code) code
#define trace_tick(args, fmt, ...) (void)0
#endif

#define trace_tick0(fmt, ...) trace_tick("%4$04X  %3$02X       %5$4s " fmt, __VA_ARGS__);
#define trace_tick1(fmt, ...) trace_tick("%4$04X  %3$02X %1$02X    %5$4s " fmt, __VA_ARGS__);
#define trace_tick2(fmt, ...) trace_tick("%4$04X  %3$02X %1$02X %2$02X %5$4s " fmt, __VA_ARGS__);

#define trace_abs(ptr) trace_tick2("$%14$04X = %15$02X                ", ptr, *cpu_map_(ptr))
#define trace_abx(ptr) trace_tick2("$%2$02X%1$02X,X @ %14$04X = %15$02X       ", ptr, *cpu_map_(ptr))
#define trace_aby(ptr) trace_tick2("$%2$02X%1$02X,Y @ %14$04X = %15$02X       ", ptr, *cpu_map_(ptr))
#define trace_acc() trace_tick0("A                         ", 0)
#define trace_imm() trace_tick1("#$%1$02X                      ", 0)
#define trace_imp() trace_tick0("                          ", 0)
#define trace_ind(ptr0, ptr1) trace_tick2("($%14$04X) = %15$04X               ", ptr0, ptr1)
#define trace_jmp(ptr) trace_tick2("$%14$04X                     ", ptr)
#define trace_ndx(ptr0, ptr1) trace_tick1("($%1$02X,X) @ %14$02X = %15$04X = %16$02X  ", ptr0, ptr1, *cpu_map_(ptr1))
#define trace_ndy(ptr0, ptr1) trace_tick1("($%1$02X),Y = %14$04X @ %15$04X = %16$02X", ptr0, ptr1, *cpu_map_(ptr1))
#define trace_rel(ptr) trace_tick1("$%14$04X                     ", ptr, *cpu_map_(ptr))
#define trace_zpg(ptr) trace_tick1("$%1$02X = %15$02X                  ", ptr, *cpu_map_(ptr))
#define trace_zpx(ptr) trace_tick1("$%1$02X,X @ %14$02X = %15$02X           ", ptr, *cpu_map_(ptr))
#define trace_zpy(ptr) trace_tick1("$%1$02X,Y @ %14$02X = %15$02X           ", ptr, *cpu_map_(ptr))

static u8 cpu_flag_zn(u8 val)
{
  if (val == 0) {
    cpu_reg.p |= CPU_FLAG_ZERO;
  } else {
    cpu_reg.p &= ~CPU_FLAG_ZERO;
  }

  if (val & 0x80) {
    cpu_reg.p |= CPU_FLAG_NEGATIVE;
  } else {
    cpu_reg.p &= ~CPU_FLAG_NEGATIVE;
  }
  return val;
}

static void cpu_flag_carry(bool cond)
{
  if (cond) {
    cpu_reg.p |= CPU_FLAG_CARRY;
  } else {
    cpu_reg.p &= ~CPU_FLAG_CARRY;
  }
}

static void cpu_flag_overflow(u8 a, u8 b, u8 res)
{
  if ((~(a ^ b) & (a ^ res)) & 0x80) {
    cpu_reg.p |= CPU_FLAG_OVERFLOW;
  } else {
    cpu_reg.p &= ~CPU_FLAG_OVERFLOW;
  }
}

static u8 cpu_read_ptr(u16 ptr) { return *cpu_map(ptr); }
static u16 cpu_read16_ptr(u16 ptr) { return (u16)cpu_read_ptr(ptr) | ((u16)cpu_read_ptr(ptr + 1) << 8); }
static u16 cpu_read16_zptr(u8 zptr) { return (u16)cpu_read_ptr(zptr) | ((u16)cpu_read_ptr((u8)(zptr + 1)) << 8); }
static void cpu_write_ptr(u16 ptr, u8 val) { *cpu_map(ptr) = val; }

static u8 cpu_op() { return cpu_read_ptr(cpu_reg.pc++); }
static u16 cpu_op16() { return (u16)cpu_op() | ((u16)cpu_op() << 8); }

static u16 cpu_addr_abs()
{
  u16 ptr = cpu_op16();
  trace_abs(ptr);
  return ptr;
}

static u16 cpu_addr_abx(bool write)
{
  u16 val = cpu_op16();
  if (write || (((u16)(u8)val + cpu_reg.x) & 0xFF00)) {
    ++ctx.cycle;
  }
  u16 ptr = val + cpu_reg.x;
  trace_abx(ptr);
  return ptr;
}

static u16 cpu_addr_aby(bool write)
{
  u16 val = cpu_op16();
  if (write || (((u16)(u8)val + cpu_reg.y) & 0xFF00)) {
    ++ctx.cycle;
  }
  u16 ptr = val + cpu_reg.y;
  trace_aby(ptr);
  return ptr;
}

static u8 cpu_read_acc()
{
  ++ctx.cycle;
  trace_acc();
  return cpu_reg.a;
}

static u8 cpu_read_imm()
{
  trace_imm();
  return cpu_op();
}

static u8 cpu_read_imp(u8 val)
{
  ++ctx.cycle;
  trace_imp();
  return val;
}

static u16 cpu_addr_ind()
{
  u16 ptr0 = cpu_op16();
  u8 lo = cpu_read_ptr(ptr0);
  u16 hi_ptr = (ptr0 & 0xFF00) | ((ptr0 + 1) & 0x00FF); // NES bug: incrementing only low byte
  u8 hi = cpu_read_ptr(hi_ptr);
  u16 ptr1 = (u16)lo | ((u16)hi << 8);
  trace_ind(ptr0, ptr1);
  return ptr1;
}

static u16 cpu_addr_jmp()
{
  u16 ptr = cpu_op16();
  trace_jmp(ptr);
  return ptr;
}

static u16 cpu_addr_ndx()
{
  ++ctx.cycle;
  u16 zptr = (u8)(cpu_op() + cpu_reg.x);
  u16 ptr = cpu_read16_zptr(zptr);
  trace_ndx(zptr, ptr);
  return ptr;
}

static u16 cpu_addr_ndy(bool write)
{
  u16 ptr0 = cpu_read16_zptr(cpu_op());
  if (write || (((u16)(u8)ptr0 + cpu_reg.y) & 0xFF00)) {
    ++ctx.cycle;
  }
  u16 ptr1 = ptr0 + cpu_reg.y;
  trace_ndy(ptr0, ptr1);
  return ptr1;
}

static u16 cpu_addr_rel()
{
  i8 val = (i8)cpu_op();
  u16 ptr = cpu_reg.pc + val;
  trace_rel(ptr);
  return ptr;
}

static u16 cpu_addr_zpg()
{
  u16 ptr = cpu_op();
  trace_zpg(ptr);
  return ptr;
}

static u16 cpu_addr_zpx()
{
  ++ctx.cycle;
  u16 ptr = (u8)(cpu_op() + cpu_reg.x);
  trace_zpx(ptr);
  return ptr;
}

static u16 cpu_addr_zpy()
{
  ++ctx.cycle;
  u16 ptr = (u8)(cpu_op() + cpu_reg.y);
  trace_zpy(ptr);
  return ptr;
}

static void cpu_stack_push(u8 val) { cpu_write_ptr(0x100 | cpu_reg.sp--, val); }

static u8 cpu_stack_pop()
{
  ++ctx.cycle;
  return cpu_read_ptr(++cpu_reg.sp | 0x100);
}

static void cpu_stack_push16(u16 val)
{
  cpu_stack_push((u8)(val >> 8));
  cpu_stack_push((u8)(val & 0xFF));
}

static u16 cpu_stack_pop16() { return (u16)cpu_stack_pop() | ((u16)cpu_stack_pop() << 8); }


static u16 cpu_addr(u8 addr, bool write)
{
  switch (addr) {
    case CPU_ADDR_NDX: return cpu_addr_ndx();
    case CPU_ADDR_ZPG: return cpu_addr_zpg();
    case CPU_ADDR_ABS: return cpu_addr_abs();
    case CPU_ADDR_NDY: return cpu_addr_ndy(write);
    case CPU_ADDR_ZPX: return cpu_addr_zpx();
    case CPU_ADDR_ABY: return cpu_addr_aby(write);
    case CPU_ADDR_ABX: return cpu_addr_abx(write);

    case CPU_ADDR_ZPY: return cpu_addr_zpy();
  }
  errorf("wrong address method %d\n", addr);
  exit(1);
  return 0;
}

static u8 cpu_read(u8 addr)
{
  switch (addr) {
    case CPU_ADDR_IMM: return cpu_read_imm();
    case CPU_ADDR_ACC: return cpu_read_acc();
    case CPU_ADDR_IMA: return cpu_read_imp(cpu_reg.a);
    case CPU_ADDR_STA: return cpu_read_imp(cpu_reg.sp);
    case CPU_ADDR_XXX: return cpu_read_imp(cpu_reg.x);
    case CPU_ADDR_YYY: return cpu_read_imp(cpu_reg.y);
  }
  return cpu_read_ptr(cpu_addr(addr, false));
}

static void cpu_write(u8 addr, u8 val)
{
  switch (addr) {
    case CPU_ADDR_IMM: trace_opcode("*NOP", cpu_read_imm()); return;
    case CPU_ADDR_ACC:
    case CPU_ADDR_IMA: cpu_reg.a = cpu_flag_zn(cpu_read_imp(val)); return;
    case CPU_ADDR_STA: cpu_reg.sp = cpu_read_imp(val); return;
    case CPU_ADDR_XXX: cpu_reg.x = cpu_flag_zn(cpu_read_imp(val)); return;
    case CPU_ADDR_YYY: cpu_reg.y = cpu_flag_zn(cpu_read_imp(val)); return;
  }
  cpu_write_ptr(cpu_addr(addr, true), val);
}

static void cpu_read_write(u8 addr, u8 (*cb)(u8))
{
  switch (addr) {
    case CPU_ADDR_NDY: trace_opcode("*KIL", exit(cpu_read_imp(1))); return;
    case CPU_ADDR_ABY: trace_opcode("*NOP", cpu_read_imp(0)); return;
    case CPU_ADDR_IMM: trace_opcode("*NOP", cpu_read_imm()); return;
    case CPU_ADDR_ACC:
    case CPU_ADDR_IMA: cpu_reg.a = cpu_flag_zn(cb(cpu_read(addr))); return;
    case CPU_ADDR_STA: cpu_reg.sp = cb(cpu_read(addr)); return;
    case CPU_ADDR_XXX: cpu_reg.x = cpu_flag_zn(cb(cpu_read(addr))); return;
    case CPU_ADDR_YYY: cpu_reg.y = cpu_flag_zn(cb(cpu_read(addr))); return;
  }
  u16 ptr = cpu_addr(addr, true);
  cpu_write_ptr(ptr, cb(cpu_read_ptr(ptr)));
  ++ctx.cycle;
}

static void cpu_adc(u8 val)
{
  u16 res = (u16)cpu_reg.a + (u16)val + (u16)(cpu_reg.p & CPU_FLAG_CARRY);
  cpu_flag_carry(res > 0xFF);
  cpu_flag_overflow(cpu_reg.a, val, (u8)res);
  cpu_reg.a = cpu_flag_zn((u8)res);
}

static void cpu_cmp(u8 reg, u8 val)
{
  cpu_flag_carry(reg >= val);
  cpu_flag_zn(reg - val);
}

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
  u8 carry = (cpu_reg.p & CPU_FLAG_CARRY);
  cpu_flag_carry(val & 0x80);
  return cpu_flag_zn((val << 1) | carry);
}

static u8 cpu_ror(u8 val)
{
  u8 carry = (cpu_reg.p & CPU_FLAG_CARRY) ? 0x80 : 0;
  cpu_flag_carry(val & 1);
  return cpu_flag_zn((val >> 1) | carry);
}

static u8 cpu_dec(u8 val) { return cpu_flag_zn(--val); }

static u8 cpu_inc(u8 val) { return cpu_flag_zn(++val); }

static void cpu_brk()
{
  cpu_read_imm();
  cpu_stack_push16(cpu_reg.pc);
  cpu_stack_push(cpu_reg.p | CPU_FLAG_BREAK | CPU_FLAG_ALWAYS_ONE);
  cpu_reg.p |= CPU_FLAG_INTERRUPT_DISABLED;
  cpu_reg.pc = cpu_read16_ptr(CPU_INT_IRQBRK);
}

static void cpu_rti()
{
  cpu_read_imp(0);
  cpu_reg.p = (cpu_stack_pop() & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE;
  cpu_reg.pc = cpu_stack_pop16();
  ctx.cycle -= 2; // sequential stack
}

static void cpu_jsr()
{
  ++ctx.cycle;
  u16 ptr = cpu_addr_jmp();
  cpu_stack_push16(cpu_reg.pc - 1);
  cpu_reg.pc = ptr;
}

static void cpu_rts()
{
  cpu_read_imp(0);
  cpu_reg.pc = cpu_stack_pop16() + 1;
}

static void cpu_branch(u8 flag, bool cond)
{
  u16 ptr = cpu_addr_rel();
  if (((cpu_reg.p & flag) != 0) == cond) {
    ++ctx.cycle;
    cpu_reg.pc = ptr;
  }
}

static void cpu_bit(u8 addr)
{
  u8 val = cpu_read(addr);
  if ((cpu_reg.a & val) == 0) {
    cpu_reg.p |= CPU_FLAG_ZERO;
  } else {
    cpu_reg.p &= ~CPU_FLAG_ZERO;
  }
  cpu_reg.p &= ~(CPU_FLAG_NEGATIVE | CPU_FLAG_OVERFLOW);
  cpu_reg.p |= (val & 0xC0);
}

static void cpu_reset()
{
  ctx.cycle = 5;

  cpu_reg.a = 0;
  cpu_reg.pc = cpu_read16_ptr(CPU_INT_RESET);
  cpu_reg.pc = 0xC000;
  cpu_reg.sp = 0xFD;
  cpu_reg.p = CPU_FLAG_ALWAYS_ONE | CPU_FLAG_INTERRUPT_DISABLED;
  cpu_reg.x = 0;
  cpu_reg.y = 0;
}


void mn_tick()
{
#ifdef MN_TRACE
  start_cycle = ctx.cycle;
  start_cpu_reg = cpu_reg;
#endif

  if (ctx.skip > 0) {
    --ctx.skip;
    return;
  }

  u8 op = cpu_op();
  u8 aaacc = (op & 0b11100000) | (op & 0b00000011);
  u8 addr = (op & 0b00011100) >> 2;

  switch (aaacc) {
    case 0b00000000: {
      switch (addr) {
        case 0: trace_opcode("BRK", cpu_brk()); break;
        case 2:
          trace_opcode("PHP", cpu_stack_push(cpu_read_imp(cpu_reg.p | CPU_FLAG_BREAK | CPU_FLAG_ALWAYS_ONE)));
          break;
        case 4: trace_opcode("BPL", cpu_branch(CPU_FLAG_NEGATIVE, 0)); break;
        case 6: trace_opcode("CLC", cpu_reg.p = cpu_read_imp(cpu_reg.p & ~CPU_FLAG_CARRY)); break;
        default: trace_opcode("*NOP", cpu_read(addr)); break;
      }
    } break;
    case 0b00100000: {
      switch (addr) {
        case 0: trace_opcode("JSR", cpu_jsr()); break;
        case 2:
          trace_opcode("PLP", cpu_reg.p = cpu_read_imp((cpu_stack_pop() & ~CPU_FLAG_BREAK) | CPU_FLAG_ALWAYS_ONE));
          break;
        case 4: trace_opcode("BMI", cpu_branch(CPU_FLAG_NEGATIVE, 1)); break;
        case 6: trace_opcode("SEC", cpu_reg.p = cpu_read_imp(cpu_reg.p | CPU_FLAG_CARRY)); break;
        case 1:
        case 3: trace_opcode("BIT", cpu_bit(addr)); break;
        default: trace_opcode("*NOP", cpu_read(addr)); break;
      }
    } break;
    case 0b01000000: {
      switch (addr) {
        case 0: trace_opcode("RTI", cpu_rti()); break;
        case 2: trace_opcode("PHA", cpu_stack_push(cpu_read_imp(cpu_reg.a))); break;
        case 4: trace_opcode("BVC", cpu_branch(CPU_FLAG_OVERFLOW, 0)); break;
        case 6: trace_opcode("CLI", cpu_reg.p = cpu_read_imp(cpu_reg.p & ~CPU_FLAG_INTERRUPT_DISABLED)); break;
        case 3: trace_opcode("JMP", cpu_reg.pc = cpu_addr_jmp()); break;
        default: trace_opcode("*NOP", cpu_read(addr)); break;
      }
    } break;
    case 0b01100000: {
      switch (addr) {
        case 0: trace_opcode("RTS", cpu_rts()); break;
        case 2: trace_opcode("PLA", cpu_reg.a = cpu_flag_zn(cpu_read_imp(cpu_stack_pop()))); break;
        case 4: trace_opcode("BVS", cpu_branch(CPU_FLAG_OVERFLOW, 1)); break;
        case 6: trace_opcode("SEI", cpu_reg.p = cpu_read_imp(cpu_reg.p | CPU_FLAG_INTERRUPT_DISABLED)); break;
        case 3: trace_opcode("JMP", cpu_reg.pc = cpu_addr_ind()); break;
        default: trace_opcode("*NOP", cpu_read(addr)); break;
      }
    } break;
    case 0b10000000: {
      switch (addr) {
        case 2: trace_opcode("DEY", cpu_read_write(CPU_ADDR_YYY, cpu_dec)); break;
        case 4: trace_opcode("BCC", cpu_branch(CPU_FLAG_CARRY, 0)); break;
        default:
          trace_op("STY");
          switch (addr) {
            case 0: addr = CPU_ADDR_IMM; break;
            case 6: trace_opcode("TYA", addr = CPU_ADDR_IMA); break;
          }
          cpu_write(addr, cpu_reg.y);
          break;
      }
    } break;
    case 0b10100000: {
      switch (addr) {
        case 4: trace_opcode("BCS", cpu_branch(CPU_FLAG_CARRY, 1)); break;
        case 6: trace_opcode("CLV", cpu_reg.p = cpu_read_imp(cpu_reg.p & ~CPU_FLAG_OVERFLOW)); break;
        default:
          trace_op("LDY");
          switch (addr) {
            case 0: addr = CPU_ADDR_IMM; break;
            case 2: trace_opcode("TAY", addr = CPU_ADDR_IMA); break;
          }
          cpu_reg.y = cpu_flag_zn(cpu_read(addr));
          break;
      }
    } break;
    case 0b11000000: {
      switch (addr) {
        case 2: trace_opcode("INY", cpu_read_write(CPU_ADDR_YYY, cpu_inc)); break;
        case 4: trace_opcode("BNE", cpu_branch(CPU_FLAG_ZERO, 0)); break;
        case 6: trace_opcode("CLD", cpu_reg.p = cpu_read_imp(cpu_reg.p & ~CPU_FLAG_DECIMAL)); break;
        case 5:
        case 7: trace_opcode("*NOP", cpu_read(addr)); break;
        case 0: addr = CPU_ADDR_IMM;
        default: trace_opcode("CPY", cpu_cmp(cpu_reg.y, cpu_read(addr))); break;
      }
    } break;
    case 0b11100000: {
      switch (addr) {
        case 2: trace_opcode("INX", cpu_read_write(CPU_ADDR_XXX, cpu_inc)); break;
        case 4: trace_opcode("BEQ", cpu_branch(CPU_FLAG_ZERO, 1)); break;
        case 6: trace_opcode("SED", cpu_reg.p = cpu_read_imp(cpu_reg.p | CPU_FLAG_DECIMAL)); break;
        case 5:
        case 7: trace_opcode("*NOP", cpu_read(addr)); break;
        case 0: addr = CPU_ADDR_IMM;
        default: trace_opcode("CPX", cpu_cmp(cpu_reg.x, cpu_read(addr))); break;
      }
    } break;
    case 0b00000001: trace_opcode("ORA", cpu_reg.a = cpu_flag_zn(cpu_reg.a | cpu_read(addr))); break;
    case 0b00100001: trace_opcode("AND", cpu_reg.a = cpu_flag_zn(cpu_reg.a & cpu_read(addr))); break;
    case 0b01000001: trace_opcode("EOR", cpu_reg.a = cpu_flag_zn(cpu_reg.a ^ cpu_read(addr))); break;
    case 0b01100001: trace_opcode("ADC", cpu_adc(cpu_read(addr))); break;
    case 0b10000001: trace_opcode("STA", cpu_write(addr, cpu_reg.a)); break;
    case 0b10100001: trace_opcode("LDA", cpu_reg.a = cpu_flag_zn(cpu_read(addr))); break;
    case 0b11000001: trace_opcode("CMP", cpu_cmp(cpu_reg.a, cpu_read(addr))); break;
    case 0b11100001: trace_opcode("SBC", cpu_adc(~cpu_read(addr))); break;
    case 0b00000010: {
      switch (addr) {
        case 2: addr = CPU_ADDR_ACC;
        default: trace_opcode("ASL", cpu_read_write(addr, cpu_asl)); break;
      }
    } break;
    case 0b00100010: {
      switch (addr) {
        case 2: addr = CPU_ADDR_ACC;
        default: trace_opcode("ROL", cpu_read_write(addr, cpu_rol)); break;
      }
    } break;
    case 0b01000010: {
      switch (addr) {
        case 2: addr = CPU_ADDR_ACC;
        default: trace_opcode("LSR", cpu_read_write(addr, cpu_lsr)); break;
      }
    } break;
    case 0b01100010: {
      switch (addr) {
        case 2: addr = CPU_ADDR_ACC;
        default: trace_opcode("ROR", cpu_read_write(addr, cpu_ror)); break;
      }
    } break;
    case 0b10000010: {
      trace_op("STX");
      switch (addr) {
        case 0: addr = CPU_ADDR_IMM; break;
        case 2: trace_opcode("TXA", addr = CPU_ADDR_IMA); break;
        case 6: trace_opcode("TXS", addr = CPU_ADDR_STA); break;
        case 5: addr = CPU_ADDR_ZPY; break;
        case 7: addr = CPU_ADDR_ABY; break;
      }
      cpu_write(addr, cpu_reg.x);
    } break;
    case 0b10100010: {
      trace_op("LDX");
      switch (addr) {
        case 0: addr = CPU_ADDR_IMM; break;
        case 2: trace_opcode("TAX", addr = CPU_ADDR_IMA); break;
        case 6: trace_opcode("TSX", addr = CPU_ADDR_STA); break;
        case 5: addr = CPU_ADDR_ZPY; break;
        case 7: addr = CPU_ADDR_ABY; break;
      }
      cpu_reg.x = cpu_flag_zn(cpu_read(addr));
    } break;
    case 0b11000010: {
      trace_op("DEC");
      switch (addr) {
        case 0: addr = CPU_ADDR_IMM; break;
        case 2: trace_opcode("DEX", addr = CPU_ADDR_XXX); break;
      }
      cpu_read_write(addr, cpu_dec);
    } break;
    case 0b11100010: {
      switch (addr) {
        case 2: trace_opcode("NOP", cpu_read_imp(0)); break;
        case 0: addr = CPU_ADDR_IMM;
        default: trace_opcode("INC", cpu_read_write(addr, cpu_inc)); break;
      }
    } break;
    // case 0b00000011: break;
    // case 0b00100011: break;
    // case 0b01000011: break;
    // case 0b01100011: break;
    // case 0b10000011: break;
    // case 0b10100011: break;
    // case 0b11000011: break;
    // case 0b11100011: break;
    default:
      errorf("unknown opcode %02X\n", op);
      exit(cpu_read_imp(1));
      break;
  }
}

bool mn_load(const char* path)
{
  FILE* f = fopen(path, "rb");
  if (!f) {
    errorf("can not open file %s: %s\n", path, strerror(errno));
    return false;
  }

  NESHeader h;
  if (!fread(&h, sizeof(h), 1, f)) {
    errorf("can not read file %s: %s\n", path, strerror(errno));
    fclose(f);
    return false;
  }
  if (h.magic[0] != 'N' || h.magic[1] != 'E' || h.magic[2] != 'S' || h.magic[3] != 0x1A) {
    errorf("not NES file %s\n", path);
    fclose(f);
    return false;
  }

  ctx.rom_pages = h.rom_pages;
  ctx.vrom_pages = h.vrom_pages;
  ctx.flags = h.flags & 0xF;
  ctx.mapper = h.mapper | (h.flags >> 4);

  if (h.rom_pages == 0 || h.rom_pages * ROM_PAGE_SIZE > sizeof(rom) || h.vrom_pages * VROM_PAGE_SIZE > sizeof(vrom)) {
    errorf("invalid size prg=%d chr=%d in %s\n", h.rom_pages, h.vrom_pages, path);
    fclose(f);
    return false;
  }

  if (fread(rom, ROM_PAGE_SIZE, h.rom_pages, f) != h.rom_pages) {
    errorf("can not read prg from %s: %s\n", path, strerror(errno));
    fclose(f);
    return false;
  }

  if (h.vrom_pages > 0) {
    if (fread(vrom, VROM_PAGE_SIZE, h.vrom_pages, f) != h.vrom_pages) {
      errorf("can not read chr from %s: %s\n", path, strerror(errno));
      fclose(f);
      return false;
    }
  }

  fclose(f);
  cpu_reset();
  return true;
}

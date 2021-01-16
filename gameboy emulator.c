// Copyright 2020. All rights reserved.
// Author: keorapetse.finger@yahoo.com (Keorapetse Finger)
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define GB

#define ROM_SIZE            0x8000
#define RAM_SIZE            0x8000
#define MAIN_MEORY_SIZE     0xffff

struct register_t {
    // 16-bit register structure:
    //
    //  +---8 bit wide----+-----8 bit wide------+
    //  |   low nibble    |     high nibble     |
    //  +---+---+-----+---+---+-----+---+---+---+
    //  | 0 | 1 | ... | 0 | 1 | ... | 1 | 1 | 0 |
    //  +---+---+-----+---+---+-----+---+---+---+
    //  |                                       |
    //  +--------------16 bit wide--------------+
    union {
        struct __attribute__((__packed__)) {
            uint8_t high;
            uint8_t low;
        };
        uint16_t data;
    };
};

struct cpu_registers_t {
    // The instructions and registers are similar to the 
    // Intel 8080, Intel 8085,Z80 microprocessors. There 
    // are six 16-bit registers and 8-bit registers. E.g,
    // 
    // The 16-bit register AF comprises two 8-bit registers, 
    // namely the higher nibble register A and lower nibble 
    // register B, each 8-bit resepectively.
    struct register_t af;
    struct register_t bc;
    struct register_t de;
    struct register_t hl;
    struct register_t sp;
    struct register_t pc;
    // The index of 8 bit registers is provided by certain
    // instructions in the intruction structure. The indeces
    // can be mapped to the 16 bit register (cpu_registers_t)
    // for easy access.
    //  +-----+-----+-----+-----+-----+-----+-----+-----+
    //  |  B  |  C  |  D  |  E  |  H  |  L  |  -  |  A  |
    //  +-----+-----+-----+-----+-----+-----+-----+-----+
    //  | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
    //  +-----+-----+-----+-----+-----+-----+-----+-----+
    uint8_t *cpu_8_bit_reg_mapping[0x08];
};

struct __attribute__((__packed__)) cpu_flags_t {
    // This bit is set when the result of a math operation is 
    // zero or two values match whenusing the CP instruction.
    uint8_t z_flag     : 1;
    // This bit is set if a subtraction was performed in the 
    // last math instuction. 
    uint8_t n_flag     : 1;
    // This bit is set if a carry occured from the low nibble
    // in the last math operation.
    uint8_t h_flag     : 1;
    // This bit is set if a carry occured from the last math 
    // operation or if register A is the smaller value when,
    // executing the CP instruction.
    uint8_t c_flag     : 1;
    uint8_t not_used   : 4;
};

struct cpu_core_t {
    struct cpu_registers_t reg;
    struct cpu_flags_t flags;
    const char *tag;
};

struct memory_t {
    union {
        struct __attribute__((__packed__)) {
            uint8_t ram[RAM_SIZE];
            uint8_t rom[ROM_SIZE];
        };
        uint16_t blocks[MAIN_MEORY_SIZE];
    };
    uint32_t size;
};

struct emulator_t {
    uint8_t opcode;

    struct cpu_core_t cpu;
    struct memory_t memory;
};

static uint8_t read_8_bit_immediate_data_from_memory(struct emulator_t *emulator) 
{
    uint16_t addr = emulator->cpu.reg.pc.data;
    emulator->cpu.reg.pc.data = emulator->cpu.reg.pc.data + 1;
    return (uint8_t) emulator->memory.blocks[addr];
}

static uint16_t read_16_bit_immediate_data_from_memory(struct emulator_t *emulator) 
{
    uint16_t addr = emulator->cpu.reg.pc.data;
    emulator->cpu.reg.pc.data = emulator->cpu.reg.pc.data + 2;
    return ((emulator->memory.blocks[addr + 1] << 8) & 0xff00) | (emulator->memory.blocks[addr] & 0xff);
}

static uint8_t read_8_bit_from_memory(struct emulator_t *emulator, uint16_t addr) 
{
    return (uint8_t) emulator->memory.blocks[addr];
}

static void write_8_bit_to_memory(struct emulator_t *emulator, uint8_t data, uint16_t addr)
{
    emulator->memory.blocks[addr] = data;
}

static void load_r_immediate_data(struct emulator_t *emulator, uint8_t dst, uint16_t addr)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    *(cpu->reg.cpu_8_bit_reg_mapping[dst]) = read_8_bit_from_memory(emulator, addr);
}

static void load_immediate_data_r(struct emulator_t *emulator, uint16_t addr, uint8_t src_reg)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    uint8_t data = *(cpu->reg.cpu_8_bit_reg_mapping[src_reg]);
    write_8_bit_to_memory(emulator, data, addr);
}

// CPU instruction set
// reference: http://bgb.bircd.org/pandocs.htm#cpuinstructionset
static void load_r_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t dst = (emulator->opcode >> 0x03) & 0x07;
    uint8_t src = emulator->opcode & 0x07;

    *(cpu->reg.cpu_8_bit_reg_mapping[dst]) = *(cpu->reg.cpu_8_bit_reg_mapping[src]);
}

static void load_r_n(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t dst = (emulator->opcode >> 0x03) & 0x07;
    *(cpu->reg.cpu_8_bit_reg_mapping[dst]) = read_8_bit_immediate_data_from_memory(emulator);
}

static void load_r_hl(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t dst = (emulator->opcode >> 0x03) & 0x07;
    load_r_immediate_data(emulator, dst, cpu->reg.hl.data);
}

static void load_hl_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t dst    = emulator->opcode & 0x07;
    uint8_t data   = *(cpu->reg.cpu_8_bit_reg_mapping[dst]);
    write_8_bit_to_memory(emulator, data, cpu->reg.hl.data);
}

static void load_hl_n(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t data = read_8_bit_immediate_data_from_memory(emulator);
    write_8_bit_to_memory(emulator, data, cpu->reg.hl.data);
}

static void add_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->reg.af.high = a + r;

    cpu->flags.n_flag = 0;
    cpu->flags.z_flag = ((a + r) == 0);
    cpu->flags.c_flag = (((a & 0xff) + (r & 0xff)) >> 0x08) & 0x01;
    cpu->flags.h_flag = (((a & 0x0f) + (r & 0x0f)) >> 0x04) & 0x01;
}

// static void adc_a_r(struct emulator_t *emulator)
// {
//     struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

//     uint8_t a      = cpu->reg.af.high;
//     uint8_t src    = emulator->opcode & 0x07;
//     uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

//     cpu->reg.af.high = a + r;

//     cpu->flags.n_flag = 0;
//     cpu->flags.z_flag = ((a + r) == 0);
//     cpu->flags.c_flag = (((a & 0xff) + (r & 0xff)) >> 0x08) & 0x01;
//     cpu->flags.h_flag = (((a & 0x0f) + (r & 0x0f)) >> 0x04) & 0x01;
// }

static void sub_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->reg.af.high = a - r;

    cpu->flags.n_flag = 1;
    cpu->flags.c_flag = r > a;
    cpu->flags.z_flag = ((a - r) == 0);
    cpu->flags.h_flag = (((a & 0x0f) - (r & 0x0f)) < 0);
}

static void and_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->reg.af.high = a & r;

    cpu->flags.n_flag = 0;
    cpu->flags.c_flag = 0;
    cpu->flags.h_flag = 1;
    cpu->flags.z_flag = ((a & r) == 0);
}

static void or_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->reg.af.high = a | r;

    cpu->flags.n_flag = 0;
    cpu->flags.c_flag = 0;
    cpu->flags.h_flag = 0;
    cpu->flags.z_flag = ((a | r) == 0);
}

static void xor_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->reg.af.high = a ^ r;

    cpu->flags.n_flag = 0;
    cpu->flags.c_flag = 0;
    cpu->flags.h_flag = 0;
    cpu->flags.z_flag = (a == r);
}

static void cp_a_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a      = cpu->reg.af.high;
    uint8_t src    = emulator->opcode & 0x07;
    uint8_t r      = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    cpu->flags.n_flag = 1;
    cpu->flags.c_flag = r > a;
    cpu->flags.z_flag = (a == r);
    cpu->flags.h_flag = (((a & 0x0f) - (r & 0x0f)) < 0);
}

static void inc_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t src = (emulator->opcode >> 0x03) & 0x07;
    uint8_t r = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    *(cpu->reg.cpu_8_bit_reg_mapping[src]) = r + 1;

    cpu->flags.n_flag = 0;
    cpu->flags.z_flag = ((r + 1) == 0);
    // cpu->flags.h_flag = (((a & 0x0f) - (r & 0x0f)) < 0);
}

static void dec_r(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t src = (emulator->opcode >> 0x03) & 0x07;
    uint8_t r = *(cpu->reg.cpu_8_bit_reg_mapping[src]);

    *(cpu->reg.cpu_8_bit_reg_mapping[src]) = r - 1;

    cpu->flags.n_flag = 1;
    cpu->flags.z_flag = ((r - 1) == 0);
    // cpu->flags.h_flag = (((a & 0x0f) - (r & 0x0f)) < 0);
}

static void rlca(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t bit_data = (cpu->reg.af.high & 0x80) != 0;
    uint8_t results  = (cpu->reg.af.high << 1) | bit_data;

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = bit_data;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rla(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t results   = (cpu->reg.af.high << 1) | cpu->flags.c_flag;
    uint8_t bit_data = (cpu->reg.af.high & 0x80) != 0;

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = bit_data;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rrca(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t bit_data = (cpu->reg.af.high & 0x01) != 0;
    uint8_t results  = (cpu->reg.af.high >> 1) | (bit_data << 0x07);

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = bit_data;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rra(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t bit_data = (cpu->reg.af.high & 0x01) != 0;
    uint8_t results  = (cpu->reg.af.high >> 1) | (bit_data << 0x07);

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = bit_data;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void emulator_initialize(struct emulator_t *emulator)
{
    // Initialize CPU registers and flags.
    emulator->cpu.reg.pc.data = 0x0000;
    emulator->cpu.reg.sp.data = 0xfffe;
#if  defined(GB) || defined(SGB)
    emulator->cpu.reg.af.data = 0xb001;
#else
    #error "[Config] error - unknown emulation platform (either GB or SGB)."
#endif
    emulator->cpu.reg.bc.data = 0x0013;
    emulator->cpu.reg.de.data = 0x00d8;
    emulator->cpu.reg.hl.data = 0x014d;
    emulator->cpu.tag = "SM83";
    
    // Map 8 bit registers
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x00] = &emulator->cpu.reg.bc.high;     // Register B
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x01] = &emulator->cpu.reg.bc.low;      // Register C
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x02] = &emulator->cpu.reg.de.high;     // Register D
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x03] = &emulator->cpu.reg.de.low;      // Register E
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x04] = &emulator->cpu.reg.hl.high;     // Register H
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x05] = &emulator->cpu.reg.hl.low;      // Register L
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x06] = NULL;                           // Not mapped
    emulator->cpu.reg.cpu_8_bit_reg_mapping[0x07] = &emulator->cpu.reg.af.high;     // Register A

    // Initialize memory and in-memory registers.
    emulator->memory.size = MAIN_MEORY_SIZE;
    memset(emulator->memory.blocks, 0, emulator->memory.size);
    emulator->memory.blocks[0xff05] = 0x00;
    emulator->memory.blocks[0xff06] = 0x00;
    emulator->memory.blocks[0xff07] = 0x00;
    emulator->memory.blocks[0xff10] = 0x80;
    emulator->memory.blocks[0xff11] = 0xbf;
    emulator->memory.blocks[0xff12] = 0xf3;
    emulator->memory.blocks[0xff14] = 0xbf;
    emulator->memory.blocks[0xff16] = 0x3f;
    emulator->memory.blocks[0xff17] = 0x00;
    emulator->memory.blocks[0xff19] = 0xbf;
    emulator->memory.blocks[0xff1a] = 0x7f;
    emulator->memory.blocks[0xff1b] = 0xff;
    emulator->memory.blocks[0xff1c] = 0x9f;
    emulator->memory.blocks[0xff1e] = 0xbf;
    emulator->memory.blocks[0xff20] = 0xff;
    emulator->memory.blocks[0xff21] = 0x00;
    emulator->memory.blocks[0xff22] = 0x00;
    emulator->memory.blocks[0xff23] = 0xbf;
    emulator->memory.blocks[0xff24] = 0x77;
    emulator->memory.blocks[0xff25] = 0xf3;
#ifdef GB
    emulator->memory.blocks[0xff26] = 0xf1;
#elif SGB
    emulator->memory.blocks[0xff26] = 0xf0;
#else
    #error "[Config] error - unknown emulation platform (either GB or SGB)."
#endif
    emulator->memory.blocks[0xff40] = 0x91;
    emulator->memory.blocks[0xff42] = 0x00;
    emulator->memory.blocks[0xff43] = 0x00;
    emulator->memory.blocks[0xff45] = 0x00;
    emulator->memory.blocks[0xff47] = 0xfc;
    emulator->memory.blocks[0xff48] = 0xff;
    emulator->memory.blocks[0xff49] = 0xff;
    emulator->memory.blocks[0xff4a] = 0x00;
    emulator->memory.blocks[0xff4b] = 0x00;
}

void step_emulator(struct emulator_t *emulator)
{
    emulator->opcode = read_8_bit_immediate_data_from_memory(emulator);
    switch (emulator->opcode) {
        // http://gcc.gnu.org/onlinedocs/gcc/Statements-implementation.html#Statements-implementation
        // 8-bit transer abd input/output instructions
        case 0x7e:
        case 0x46:
        case 0x4e:
        case 0x56:
        case 0x5e:
        case 0x66:
        case 0x6e:
            load_r_hl(emulator);
            break;
        case 0x06:
        case 0x0e:
        case 0x16:
        case 0x1e:
        case 0x26:
        case 0x2e:
            load_r_n(emulator);
            break;
        case 0x7f:              // LD A, r'
        case 0x78 ... 0x7d:
        case 0x40 ... 0x45:     // LD B, r'
        case 0x48:              // LD C, r'
        case 0x49:
        case 0x4a ... 0x4d:
        case 0x50 ... 0x55:     // LD D, r'
        case 0x58:              // LD E, r'
        case 0x59:
        case 0x5a ... 0x5d:
        case 0x60 ... 0x65:     // LD H, r'
        case 0x68:              // LD L, r'
        case 0x69:
        case 0x6a ... 0x6d:
            load_r_r(emulator);
            break;
        case 0x70 ... 0x75:     // LD (HL), r'
            load_hl_r(emulator);
            break;
        case 0x36:
            load_hl_n(emulator);
            break;
        case 0x0a:
            load_r_immediate_data(emulator, 0x07, emulator->cpu.reg.bc.data);
            break;
        case 0x1a:
            load_r_immediate_data(emulator, 0x07, emulator->cpu.reg.de.data);
            break;
        case 0xf2: {
            uint16_t addr = 0xff00 + emulator->cpu.reg.bc.low;
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xe2: {
            uint16_t addr = 0xff00 + emulator->cpu.reg.bc.low;
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0xf0: {
            uint8_t addr = read_8_bit_immediate_data_from_memory(emulator);
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xe0: {
            uint8_t addr = read_8_bit_immediate_data_from_memory(emulator);
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0xfa: {
            uint16_t addr = read_16_bit_immediate_data_from_memory(emulator);
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xea: {
            uint16_t addr = read_16_bit_immediate_data_from_memory(emulator);
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0x2a: {
            load_r_immediate_data(emulator, 0x07, emulator->cpu.reg.hl.data);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data + 1;
            break;
        }
        case 0x3a: {
            load_r_immediate_data(emulator, 0x07, emulator->cpu.reg.hl.data);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data - 1;
            break;
        }
        case 0x02:
            load_immediate_data_r(emulator, emulator->cpu.reg.bc.data, 0x07);
            break;
        case 0x12:
            load_immediate_data_r(emulator, emulator->cpu.reg.de.data, 0x07);
            break;
        case 0x22: {
            load_immediate_data_r(emulator, emulator->cpu.reg.hl.data, 0x07);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data + 1;
            break;
        }
        case 0x32: {
            load_immediate_data_r(emulator, emulator->cpu.reg.hl.data, 0x07);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data - 1;
            break;
        }
        // 8-bit arithmetic and logic operation instructions
        // TDOD. adc, sbc
        case 0x87:
        case 0x80 ... 0x85:
            add_a_r(emulator);
            break;
        case 0x97:
        case 0x90 ... 0x95:
            sub_a_r(emulator);
            break;
        case 0xa7:
        case 0xa0 ... 0xa5:
            and_a_r(emulator);
            break;
        case 0xb7:
        case 0xb0 ... 0xb5:
            or_a_r(emulator);
            break;
        case 0xaf:
        case 0xa8:
        case 0xa9:
        case 0xaa ... 0xad:
            xor_a_r(emulator);
            break;
        case 0xbf:
        case 0xb8 ... 0xbd:
            cp_a_r(emulator);
            break;
        case 0x04:
        case 0x0c:
        case 0x14:
        case 0x1c:
        case 0x3c:
        case 0x24:
        case 0x2c:
            inc_r(emulator);
            break;
        case 0x3d:
        case 0x05:
        case 0x0d:
        case 0x15:
        case 0x1d:
        case 0x25:
        case 0x2d:
            dec_r(emulator);
            break;
        // Roatate shift instructions
        case 0x07:
            rlca(emulator);
            break;
        case 0x17:
            rla(emulator);
            break;
        case 0x0f:
            rrca(emulator);
            break;
        case 0x1f:
            rra(emulator);
            break;
        default:
            break;
    }
}

void dum_cpu_registers(struct emulator_t *emulator)
{
    printf("A = %2xh,\t",   emulator->cpu.reg.af.high);
    printf("B = %2xh,\t",   emulator->cpu.reg.bc.high);
    printf("D = %2xh,\t",   emulator->cpu.reg.de.high);
    printf("H = %2xh\n",    emulator->cpu.reg.hl.high);
    printf("- = %2xh,\t",   emulator->cpu.reg.af.low);
    printf("C = %2xh,\t",   emulator->cpu.reg.bc.low);
    printf("E = %2xh,\t",   emulator->cpu.reg.de.low);
    printf("L = %2xh\n\n",  emulator->cpu.reg.hl.low);
    printf("PC= %2xh\n\n",  emulator->cpu.reg.pc.data);
    printf("Z = %2xh,\t",   emulator->cpu.flags.z_flag);
    printf("N = %2xh,\t",   emulator->cpu.flags.n_flag);
    printf("H = %2xh,\t",   emulator->cpu.flags.h_flag);
    printf("C = %2xh\n",    emulator->cpu.flags.c_flag);

}

// SDL2 https://lazyfoo.net/tutorials/SDL/01_hello_SDL/mac/index.php
int main(int argc, char *argv[]) 
{
    struct emulator_t emulator;

    emulator_initialize(&emulator);

    emulator.memory.blocks[0]       = 0xfa;     // ld a, (nn)
    emulator.memory.blocks[1]       = 0x55;
    emulator.memory.blocks[2]       = 0x77;
    emulator.memory.blocks[0x7755]  = 0x1e;
    emulator.memory.blocks[3]       = 0x80;     // add a, b
    emulator.memory.blocks[4]       = 0x90;     // sub a, b
    emulator.memory.blocks[5]       = 0xa1;     // and a, c
    emulator.memory.blocks[6]       = 0xb0;     // or  a, b
    emulator.memory.blocks[7]       = 0xaf;     // xor a, a
    emulator.memory.blocks[8]       = 0x3c;     // inc a
    emulator.memory.blocks[9]       = 0x05;     // dec b
    emulator.memory.blocks[10]      = 0xb8;     // cp  a, b
    emulator.memory.blocks[11]      = 0x07;     // rlca
    emulator.memory.blocks[12]      = 0x17;     // rla
    emulator.memory.blocks[13]      = 0x0f;     // rrca
    emulator.memory.blocks[14]      = 0x1f;     // rra

    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);

    dum_cpu_registers(&emulator);
    return 0;
}

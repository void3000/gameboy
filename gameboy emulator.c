// Copyright 2020. All rights reserved.
// Author: keorapetse.finger@yahoo.com (Keorapetse Finger)
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
            uint8_t low;
            uint8_t high;
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
            uint8_t rom[ROM_SIZE];
            uint8_t ram[RAM_SIZE];
        };
        uint8_t blocks[MAIN_MEORY_SIZE];
    };
    uint16_t size;
};

struct emulator_t {
    uint8_t opcode;

    struct cpu_core_t cpu;
    struct memory_t memory;
};

uint8_t boot_rom[0x0100] =
{
    // Gameboy Bootstrap ROM - https://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E, 
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0, 
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B, 
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9, 
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04, 
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2, 
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06, 
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17, 
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C, 
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20, 
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

static uint8_t read_8_bit_immediate_data_from_memory(struct emulator_t *emulator) 
{
    uint16_t addr = emulator->cpu.reg.pc.data;
    emulator->cpu.reg.pc.data = emulator->cpu.reg.pc.data + 1;
    return emulator->memory.blocks[addr];
}

static uint16_t read_16_bit_immediate_data_from_memory(struct emulator_t *emulator) 
{
    uint16_t addr = emulator->cpu.reg.pc.data;
    emulator->cpu.reg.pc.data = emulator->cpu.reg.pc.data + 2;
    return ((emulator->memory.blocks[addr + 1] << 8) & 0xff00) | (emulator->memory.blocks[addr] & 0xff);
}

static uint8_t read_8_bit_from_memory(struct emulator_t *emulator, uint16_t addr) 
{
    return emulator->memory.blocks[addr];
}

static uint16_t read_16_bit_from_memory(struct emulator_t *emulator, uint16_t addr) 
{
    return (((emulator->memory.blocks[addr + 1] << 8) & 0xff00) | (emulator->memory.blocks[addr] & 0xff)) & 0xffff;
}

static void write_8_bit_to_memory(struct emulator_t *emulator, uint8_t data, uint16_t addr)
{
    emulator->memory.blocks[addr] = data;
}

static void write_16_bit_to_memory(struct emulator_t *emulator, uint16_t data, uint16_t addr)
{
    emulator->memory.blocks[addr] = data & 0xff;
    emulator->memory.blocks[addr + 1] = (data >> 0x08) & 0xff;
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

static void cp_a_n(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t a_reg = cpu->reg.af.high;
    uint8_t data = read_8_bit_immediate_data_from_memory(emulator);

    cpu->flags.n_flag = 1;
    cpu->flags.c_flag = data > a_reg;
    cpu->flags.z_flag = (a_reg == data);
    cpu->flags.h_flag = (((a_reg & 0x0f) - (data & 0x0f)) < 0);
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

static void inc_rr(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    switch (emulator->opcode)
    {
        case 0x03:
            cpu->reg.bc.data = cpu->reg.bc.data - 1;
            break;
        case 0x13:
            cpu->reg.de.data = cpu->reg.de.data - 1;
            break;
        case 0x23:
            cpu->reg.hl.data = cpu->reg.hl.data - 1;
            break;
        case 0x33:
            cpu->reg.sp.data = cpu->reg.sp.data - 1;
            break;
        default:
            break;
    }
}

static void dec_rr(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    switch (emulator->opcode)
    {
        case 0x0b:
            cpu->reg.bc.data = cpu->reg.bc.data - 1;
            break;
        case 0x1b:
            cpu->reg.de.data = cpu->reg.de.data - 1;
            break;
        case 0x2b:
            cpu->reg.hl.data = cpu->reg.hl.data - 1;
            break;
        case 0x3b:
            cpu->reg.sp.data = cpu->reg.sp.data - 1;
            break;
        default:
            break;
    }
}

static void rlca(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t carry_bit = (cpu->reg.af.high & 0x80) != 0;
    uint8_t results  = (cpu->reg.af.high << 1) | carry_bit;

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = carry_bit;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rla(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t results   = (cpu->reg.af.high << 1) | cpu->flags.c_flag;
    uint8_t carry_bit = (cpu->reg.af.high & 0x80) != 0;

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = carry_bit;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rrca(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t carry_bit = (cpu->reg.af.high & 0x01) != 0;
    uint8_t results  = (cpu->reg.af.high >> 1) | (carry_bit << 0x07);

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = carry_bit;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void rra(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t carry_bit = (cpu->reg.af.high & 0x01) != 0;
    uint8_t results  = (cpu->reg.af.high >> 1) | (cpu->flags.c_flag << 0x07);

    cpu->flags.z_flag = results == 0;
    cpu->flags.c_flag = carry_bit;
    cpu->flags.n_flag = 0;
    cpu->flags.h_flag = 0;

    cpu->reg.af.high = results;
}

static void bit_operations(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    uint8_t data = read_8_bit_immediate_data_from_memory(emulator);
    uint8_t r = data & 0x07;

    uint8_t save_result = 1;
    uint8_t affect_flags= 1;
    uint8_t results;

    uint8_t carry_bit = cpu->flags.c_flag;
    uint8_t h_flag = cpu->flags.h_flag;

    // Skipped rlc (hl) instruction
    switch(data)
    {
        case 0x07:
        case 0x00 ... 0x05:  // RLC r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x80) != 0;
            results = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) << 1) | carry_bit;
            break;
        }
        case 0x17:
        case 0x10 ... 0x15: // RL r
        {
            results  = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) << 1) | cpu->flags.c_flag;
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x80) != 0;
            break;
        }
        case 0x0f:
        case 0x08 ... 0x0d:  // RRC r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x01) != 0;
            results = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) >> 1) | (carry_bit << 0x07);
            break;
        }
        case 0x1f:
        case 0x18 ... 0x1d:  // RR  r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x01) != 0;
            results  = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) >> 1) | (cpu->flags.c_flag << 0x07);
            break;
        }
        case 0x27:
        case 0x20 ... 0x25: // SLA r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x80) != 0;
            results = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) << 1);
            break;
        }
        case 0x2f:
        case 0x28 ... 0x2d: // SRA r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x01) != 0;
            results = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) >> 1) | ((*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x80));
            break;
        }
        case 0x3f:
        case 0x38 ... 0x3d: // SRL r
        {
            carry_bit = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) & 0x01) != 0;
            results = (*(cpu->reg.cpu_8_bit_reg_mapping[r]) >> 1) & 0xff;
            break;
        }
        case 0x40 ... 0x45: // BIT b, r
        case 0x47 ... 0x4d:
        case 0x4f:
        case 0x50 ... 0x55:
        case 0x57 ... 0x5d:
        case 0x5f:
        case 0x60 ... 0x65:
        case 0x67 ... 0x6d:
        case 0x6f:
        case 0x70 ... 0x75:
        case 0x77 ... 0x7d:
        case 0x7f:
        {
            uint8_t index = (data >> 0x03 ) & 0x07;
            results = *(cpu->reg.cpu_8_bit_reg_mapping[r]) & (1 << index);
            save_result = 0;
            h_flag = 1;
            break;
        }
        case 0x80 ... 0x85: // RES b, r
        case 0x87 ... 0x8d:
        case 0x8f:
        case 0x90 ... 0x95:
        case 0x97 ... 0x9d:
        case 0x9f:
        case 0xa0 ... 0xa5:
        case 0xa7 ... 0xad:
        case 0xaf:
        case 0xb0 ... 0xb5:
        case 0xb7 ... 0xbd:
        case 0xbf:
        {
            uint8_t index = (data >> 0x03 ) & 0x07;
            results = *(cpu->reg.cpu_8_bit_reg_mapping[r]) & ~(1 << index); 
            save_result = 1;
            affect_flags = 0;
            break;
        }
        case 0xc0 ... 0xc5: // SET b, r
        case 0xc7 ... 0xcd:
        case 0xcf:
        case 0xd0 ... 0xd5:
        case 0xd7 ... 0xdd:
        case 0xdf:
        case 0xe0 ... 0xe5:
        case 0xe7 ... 0xed:
        case 0xef:
        case 0xf0 ... 0xf5:
        case 0xf7 ... 0xfd:
        case 0xff:
        {
            uint8_t index = (data >> 0x03 ) & 0x07;
            results = *(cpu->reg.cpu_8_bit_reg_mapping[r]) | (1 << index); 
            save_result = 1;
            affect_flags = 0;
            break;
        }
        defualt:
            break;
    }

    if (affect_flags)
    {
        cpu->flags.z_flag = results == 0;
        cpu->flags.c_flag = carry_bit;
        cpu->flags.n_flag = 0;
        cpu->flags.h_flag = h_flag;
    }

    if (save_result) *(cpu->reg.cpu_8_bit_reg_mapping[r]) = results;
}

static void ld_rr_nn(struct emulator_t *emulator)
{
    uint16_t immediate_data = read_16_bit_immediate_data_from_memory(emulator);
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;

    switch (emulator->opcode)
    {
        case 0x01:
            cpu->reg.bc.data = immediate_data;
            break;
        case 0x11:
            cpu->reg.de.data = immediate_data;
            break;
        case 0x21:
            cpu->reg.hl.data = immediate_data;
            break;
        case 0x31:
            cpu->reg.sp.data = immediate_data;
            break;
        default:
            break;
    }
}

static void jump_nn(struct emulator_t *emulator, uint16_t addr)
{
    emulator->cpu.reg.pc.data = addr;
}

static void jump_n(struct emulator_t *emulator, uint8_t addr)
{
    emulator->cpu.reg.pc.data = emulator->cpu.reg.pc.data + (int8_t)addr;
}

static void jump_cc_nn(struct emulator_t *emulator, uint16_t addr)
{
    uint8_t should_jump = 0;
    switch (emulator->opcode)
    {
        case 0xc2: should_jump = !emulator->cpu.flags.z_flag; break;
        case 0xca: should_jump = emulator->cpu.flags.z_flag;  break;
        case 0xd2: should_jump = !emulator->cpu.flags.c_flag; break;
        case 0xda: should_jump = emulator->cpu.flags.c_flag;  break;
    }
    if (should_jump) jump_nn(emulator, addr);
}

static void jump_cc_n(struct emulator_t *emulator, uint8_t addr)
{
    uint8_t should_jump = 0;
    switch (emulator->opcode)
    {
        case 0x20: should_jump = !emulator->cpu.flags.z_flag; break;
        case 0x28: should_jump = emulator->cpu.flags.z_flag;  break;
        case 0x30: should_jump = !emulator->cpu.flags.c_flag; break;
        case 0x38: should_jump = emulator->cpu.flags.c_flag;  break;
    }
    if (should_jump) jump_n(emulator, addr);
}

static void call_nn(struct emulator_t *emulator, uint16_t addr)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    
    write_16_bit_to_memory(emulator, cpu->reg.pc.data, cpu->reg.sp.data);
    cpu->reg.pc.data = addr;
    cpu->reg.sp.data = cpu->reg.sp.data - 2;
}

static void call_cc_nn(struct emulator_t *emulator, uint16_t addr)
{
    uint8_t should_jump = 0;
    switch (emulator->opcode)
    {
        case 0xc4: should_jump = !emulator->cpu.flags.z_flag; break;
        case 0xcc: should_jump = emulator->cpu.flags.z_flag;  break;
        case 0xd4: should_jump = !emulator->cpu.flags.c_flag; break;
        case 0xdc: should_jump = emulator->cpu.flags.c_flag;  break;
    }
    if (should_jump) call_nn(emulator, addr);
}

static void push_qq(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    uint16_t data;

    switch (emulator->opcode)
    {
        case 0xc5:
            data = cpu->reg.bc.data;
            break;
        case 0xd5:
            data = cpu->reg.de.data;
            break;
        case 0xe5:
            data = cpu->reg.hl.data;
            break;
        case 0xf5:
            data = cpu->reg.af.data;    // fix unloaded F register
            break;
        default:
            exit(0);
    }
    write_16_bit_to_memory(emulator, data, cpu->reg.sp.data);
    cpu->reg.sp.data = cpu->reg.sp.data - 2;
}

static void pop_qq(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    
    cpu->reg.sp.data = cpu->reg.sp.data + 2;
    uint16_t data = read_16_bit_from_memory(emulator, cpu->reg.sp.data);
    switch (emulator->opcode)
    {
        case 0xc1:
            cpu->reg.bc.data = data;
            break;
        case 0xd1:
            cpu->reg.de.data = data;
            break;
        case 0xe1:
            cpu->reg.hl.data = data;
            break;
        case 0xf1:
            cpu->reg.af.data = data; // fix unloaded F register
            break;
        default:
            exit(0);
    }
}

static void ret(struct emulator_t *emulator)
{
    struct cpu_core_t *cpu = (struct cpu_core_t*) &emulator->cpu;
    cpu->reg.sp.data = cpu->reg.sp.data + 2;
    cpu->reg.pc.data = read_16_bit_from_memory(emulator, cpu->reg.sp.data);
}

static void ret_cc(struct emulator_t *emulator)
{
    uint8_t should_jump = 0;
    switch (emulator->opcode)
    {
        case 0xc0: should_jump = !emulator->cpu.flags.z_flag; break;
        case 0xc8: should_jump = emulator->cpu.flags.z_flag;  break;
        case 0xd0: should_jump = !emulator->cpu.flags.c_flag; break;
        case 0xd8: should_jump = emulator->cpu.flags.c_flag;  break;
    }
    if (should_jump) ret(emulator);
}

static void emulator_initialize(struct emulator_t *emulator)
{
    // Initialize CPU registers and flags. http://bgb.bircd.org/pandocs.htm#powerupsequence
    emulator->cpu.reg.pc.data = 0x0000;
    emulator->cpu.reg.sp.data = 0xfffe;
#if  defined(GB) || defined(SGB)
    emulator->cpu.reg.af.data = 0x01b0;
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

    // Initialize memory and in-memory registers. http://bgb.bircd.org/pandocs.htm#powerupsequence
    emulator->memory.size = MAIN_MEORY_SIZE;
    memset(emulator->memory.blocks, 0, emulator->memory.size);
    memcpy(emulator->memory.rom, boot_rom, 0x0100);

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

void dum_cpu_registers(struct emulator_t *emulator)
{
    printf("[INFO ] Register dumps\n");
    printf("A = %2xh,\t",   emulator->cpu.reg.af.high);
    printf("B = %2xh,\t",   emulator->cpu.reg.bc.high);
    printf("D = %2xh,\t",   emulator->cpu.reg.de.high);
    printf("H = %2xh\n",    emulator->cpu.reg.hl.high);
    printf("- = %2xh,\t",   emulator->cpu.reg.af.low);
    printf("C = %2xh,\t",   emulator->cpu.reg.bc.low);
    printf("E = %2xh,\t",   emulator->cpu.reg.de.low);
    printf("L = %2xh\n\n",  emulator->cpu.reg.hl.low);
    printf("PC= %2xh,\t",   emulator->cpu.reg.pc.data);
    printf("SP= %2xh\n\n",  emulator->cpu.reg.sp.data);
    printf("Z = %2xh,\t",   emulator->cpu.flags.z_flag);
    printf("N = %2xh,\t",   emulator->cpu.flags.n_flag);
    printf("H = %2xh,\t",   emulator->cpu.flags.h_flag);
    printf("C = %2xh\n",    emulator->cpu.flags.c_flag);
    printf("[INFO ] End\n\n");
}

void step_emulator(struct emulator_t *emulator)
{
    emulator->opcode = read_8_bit_immediate_data_from_memory(emulator);
    printf("[DEBUG] Executing opcode = $%x\n", emulator->opcode);
    switch (emulator->opcode) 
    {
        // http://gcc.gnu.org/onlinedocs/gcc/Statements-implementation.html#Statements-implementation
        // Opcodes - https://gbdev.io/gb-opcodes/optables/
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
        case 0x3e:
            load_r_n(emulator);
            break;
        case 0x7f:              // LD A, r'
        case 0x78 ... 0x7d:
        case 0x40 ... 0x45:     // LD B, r'
        case 0x47:
        case 0x48:              // LD C, r'
        case 0x49:
        case 0x4f:
        case 0x4a ... 0x4d:
        case 0x50 ... 0x55:     // LD D, r'
        case 0x57:
        case 0x58:              // LD E, r'
        case 0x59:
        case 0x5a ... 0x5d:
        case 0x60 ... 0x65:     // LD H, r'
        case 0x67:
        case 0x68:              // LD L, r'
        case 0x69:
        case 0x6a ... 0x6d:
            load_r_r(emulator);
            break;
        case 0x70 ... 0x75:     // LD (HL), r'
        case 0x77:
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
        case 0xf2: 
        {
            uint16_t addr = 0xff00 + emulator->cpu.reg.bc.low;
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xe2: 
        {
            uint16_t addr = 0xff00 + emulator->cpu.reg.bc.low;
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0xf0: 
        {
            uint8_t addr = read_8_bit_immediate_data_from_memory(emulator);
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xe0: 
        {
            uint8_t addr = read_8_bit_immediate_data_from_memory(emulator);
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0xfa: 
        {
            uint16_t addr = read_16_bit_immediate_data_from_memory(emulator);
            load_r_immediate_data(emulator, 0x07, addr);
            break;
        }
        case 0xea: 
        {
            uint16_t addr = read_16_bit_immediate_data_from_memory(emulator);
            load_immediate_data_r(emulator, addr, 0x07);
            break;
        }
        case 0x2a: 
        {
            load_r_immediate_data(emulator, 0x07, emulator->cpu.reg.hl.data);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data + 1;
            break;
        }
        case 0x3a: 
        {
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
        case 0x22: 
        {
            load_immediate_data_r(emulator, emulator->cpu.reg.hl.data, 0x07);
            emulator->cpu.reg.hl.data = emulator->cpu.reg.hl.data + 1;
            break;
        }
        case 0x32: 
        {
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
        case 0xfe:  // CP a, n
            cp_a_n(emulator);
            break;
        case 0xbf:  // CP a, r
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
        // Extended instructions
        case 0xcb:
            // RLC r
            // RL  r
            // RRC r
            // RR  r
            // SLA r
            // SRA r
            // SRL r
            // BIT b, r
            bit_operations(emulator);
            break;
        // Jump instructions
        case 0xc3:  // JP nn
            jump_nn(emulator, read_16_bit_immediate_data_from_memory(emulator));
            break;
        case 0xc2:
        case 0xca:
        case 0xd2:
        case 0xda:  // JP cc, nn
            jump_cc_nn(emulator, read_16_bit_immediate_data_from_memory(emulator));
            break;
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:  // JR cc, n
            jump_cc_n(emulator, read_8_bit_immediate_data_from_memory(emulator));
            break;
        case 0x18:  // JR n
            jump_n(emulator, read_8_bit_immediate_data_from_memory(emulator));
            break;
        case 0xe9:  // JP (hl)
            jump_nn(emulator, emulator->cpu.reg.hl.data);
            break;
        // Call instructions
        case 0xcd:  // Call nn
            call_nn(emulator, read_16_bit_immediate_data_from_memory(emulator));
            break;
        case 0xc4:  // Call cc, nn
        case 0xcc:
        case 0xd4:
        case 0xdc:
            call_cc_nn(emulator, read_16_bit_immediate_data_from_memory(emulator));
            break;
        case 0xc9:  // RET
            ret(emulator);
            break;
        case 0xc0:
        case 0xc8:
        case 0xd0:
        case 0xd8:
            ret_cc(emulator);
            break;
        // 16 bit transfer instructions
        case 0x01:  // LD rr, nn
        case 0x11:
        case 0x21:
        case 0x31:
            ld_rr_nn(emulator);
            break;
        case 0xc5:  // Push rr
        case 0xd5:
        case 0xe5:
        case 0xf5:
            push_qq(emulator);
            break;
        case 0xc1:  // Pop rr
        case 0xd1:
        case 0xe1:
        case 0xf1:
            pop_qq(emulator);
            break;
        case 0xf9:
            emulator->cpu.reg.sp.data = emulator->cpu.reg.hl.data;
            break;
        // 16 bit atirthmetic
        case 0x03:  // INC rr
        case 0x13:
        case 0x23:
        case 0x33:
            inc_rr(emulator);
            break;
        case 0x0b:  // DEC rr
        case 0x1b:
        case 0x2b:
        case 0x3b:
            dec_rr(emulator);
            break;
        default:
        {
            printf("[DEBUG] Instruction $%x Not Implemented.\n", emulator->opcode);
            dum_cpu_registers(emulator);
            exit(0);
        }
    }
}

// SDL2 https://lazyfoo.net/tutorials/SDL/01_hello_SDL/mac/index.php
// Boot sequence https://knight.sc/reverse%20engineering/2018/11/19/game-boy-boot-sequence.html
int main(int argc, char *argv[]) 
{
    struct emulator_t emulator;

    emulator_initialize(&emulator);
    dum_cpu_registers(&emulator);
    
    step_emulator(&emulator);
    step_emulator(&emulator);
    step_emulator(&emulator);
    
    for (int k = 0; ; ++k)
    {
        step_emulator(&emulator);
    }

    dum_cpu_registers(&emulator);

    return 0;
}

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "instruction.h"

#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)
#define BIT_SET(a, n, on) { if (on) a |= (1 << n); else a &= ~(1 << n);}

// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page


typedef struct
{
    uint8_t A; uint8_t F;
    uint8_t B; uint8_t C;
    uint8_t D; uint8_t E;
    uint8_t H; uint8_t L;
    uint16_t PC;
    uint16_t SP;
} Registers;

uint8_t *rom_buffer;

typedef struct {
    Registers regs;
    uint16_t fetched_data;
    uint16_t mem_dst;
    bool dst_is_mem;
    uint8_t curr_op;
    Instruction *curr_ins;
    bool halt;
} CPU;

CPU cpu = {0};

void cpu_set_flags(char z, char n, char h, char c) {
    if (z != -1) {
        BIT_SET(cpu.regs.F, 7, z);
    }

    if (n != -1) {
        BIT_SET(cpu.regs.F, 6, n);
    }

    if (h != -1) {
        BIT_SET(cpu.regs.F, 5, h);
    }

    if (c != -1) {
        BIT_SET(cpu.regs.F, 4, c);
    }
}

Instruction instructions[0x100] = {
    [0x00] = {IN_NOP, "NOP", AM_IMP},
    [0x05] = {IN_DEC, "DEC B", AM_R, REG_B},
    [0x06] = {IN_LD, "LD B, d8", AM_R_D8, REG_B},
    [0x0e] = {IN_LD, "LD C, d8", AM_R_D8, REG_C},
    [0x20] = {IN_JP, "JR NZ, s8", AM_D8, 0, 0, CT_NZ},
    [0x21] = {IN_LD, "LD HL, d16", AM_R_D16, REG_HL},
    [0x32] = {IN_LD, "LD (HL-), A", AM_HLD_R, REG_HL, REG_A},
    [0xaf] = {IN_XOR, "XOR A, A", AM_R, REG_A},
    [0xc3] = {IN_JP, "JP a16", AM_D16}
};

uint8_t bus_read(uint16_t addr)
{
    if (addr < 0x8000) {
        return rom_buffer[addr];
    }

    printf("Unimpl bus_read above 0x8000");
    assert(0);
    return 0;
}

void bus_write(uint16_t addr, uint8_t byte)
{
    if (addr < 0x8000) {
        printf("Cannot write to rom memory!");
        assert(0);
    }
    if (addr >= 0x8000 && addr < 0xE000) {
        rom_buffer[addr] = byte;
        return;
    }

    printf("Unimpl bus_read above 0xE000 or below 0x8000");
    assert(0);
}

uint16_t bus_read16(uint16_t address) {
    uint16_t lo = bus_read(address);
    uint16_t hi = bus_read(address + 1);

    return lo | (hi << 8);
}

void bus_write16(uint16_t address, uint16_t value) {
    bus_write(address + 1, (value >> 8) & 0xFF);
    bus_write(address, value & 0xFF);
}

void cycles(uint8_t n) {}

uint16_t cpu_read_reg(reg_type rt)
{
    switch (rt)
    {
    case REG_A: return cpu.regs.A;
    case REG_B: return cpu.regs.B;
    case REG_C: return cpu.regs.C;
    case REG_HL: return (cpu.regs.H << 8) | cpu.regs.L;

    default:
        printf("register unimplemented\n");
        assert(0);
        break;
    }
}

void cpu_set_reg(reg_type rt, uint16_t v)
{
    switch (rt)
    {
    case REG_A: cpu.regs.A = v & 0xff; break;
    case REG_B: cpu.regs.B = v & 0xff; break;
    case REG_C: cpu.regs.C = v & 0xff; break;
    case REG_HL: {
        cpu.regs.H = (v >> 8) & 0xff;
        cpu.regs.L = v & 0xff;
        break;
    }

    default:
        printf("register unimplemented\n");
        assert(0);
        break;
    }
}

Instruction *get_ins_from_opcode(uint8_t opcode)
{
    if (instructions[opcode].kind == IN_INVALID) {
        return NULL;
    }
    return &instructions[opcode];
}

void fetch_ins()
{
    cpu.curr_op = bus_read(cpu.regs.PC++);
    cpu.curr_ins = get_ins_from_opcode(cpu.curr_op);
    if (cpu.curr_ins == NULL) {
        printf("unknown instruction 0x%x\n", cpu.curr_op);
        assert(0);
    }
}

void fetch_data()
{
    cpu.mem_dst = 0;
    cpu.dst_is_mem = false;

    switch (cpu.curr_ins->mode)
    {
    case AM_IMP: return;

    case AM_R: {
        cpu.fetched_data = cpu_read_reg(cpu.curr_ins->reg1);
        return;
    }

    case AM_R_D8: {
        cpu.fetched_data = bus_read(cpu.regs.PC);
        cpu.regs.PC++;
        cycles(1);
        return;
    }
    
    case AM_D8: {
        cpu.fetched_data = bus_read(cpu.regs.PC);
        cycles(1);
        cpu.regs.PC++;
        return;
    }

    case AM_D16: {
        uint16_t lo = bus_read(cpu.regs.PC);
        uint16_t hi = bus_read(cpu.regs.PC + 1);
        cpu.fetched_data = (hi << 8) | lo;
        cpu.regs.PC += 2;
        cycles(2);
        return;
    }

    case AM_R_D16: {
        uint16_t lo = bus_read(cpu.regs.PC);
        uint16_t hi = bus_read(cpu.regs.PC + 1);
        cpu.fetched_data = (hi << 8) | lo;
        cpu.regs.PC += 2;
        cycles(2);
        break;
    }

    case AM_HLI_R: {
        cpu.fetched_data = cpu_read_reg(cpu.curr_ins->reg2);
        cpu.mem_dst = cpu_read_reg(cpu.curr_ins->reg1);
        cpu.dst_is_mem = true;
        cpu_set_reg(REG_HL, cpu_read_reg(REG_HL) + 1);
        break;
    }

    case AM_HLD_R: {
        cpu.fetched_data = cpu_read_reg(cpu.curr_ins->reg2);
        cpu.mem_dst = cpu_read_reg(cpu.curr_ins->reg1);
        cpu.dst_is_mem = true;
        cpu_set_reg(REG_HL, cpu_read_reg(REG_HL) - 1);
        break;
    }

    default:
        printf("unknown addressing mode %d", cpu.curr_ins->mode);
        assert(0);
        break;
    }
}

void execute()
{
    switch (cpu.curr_ins->kind)
    {
    case IN_DEC:
        cpu.regs.B--;
        cpu_set_flags(cpu.regs.B, 0, 0, 0);
        break;
    case IN_XOR:
        cpu.regs.A ^= cpu.fetched_data & 0xff;
        cpu_set_flags(cpu.regs.A, 0, 0, 0);
        break;
    
    case IN_LD:
        if (cpu.dst_is_mem) {
            // LD (BC), A
            reg_type r2 = cpu.curr_ins->reg2;
            switch (r2) {
                case REG_A:
                case REG_B:
                case REG_C:
                    bus_write(cpu.mem_dst, cpu.fetched_data);
                    break;
                case REG_HL:
                    bus_write16(cpu.mem_dst, cpu.fetched_data);
                    break;
                default:
                    printf("unimplemented for this reg.\n");
                    assert(0);
            }
            return;
        }

        if (cpu.curr_ins->mode == AM_HL_SPR) {
            assert(0);
        }

        cpu_set_reg(cpu.curr_ins->reg1, cpu.fetched_data);
        break;

    case IN_JP:
        switch (cpu.curr_ins->cond) {
            case CT_NZ:
                if (BIT(cpu.regs.F, 7)) {
                    
                }
                else {
                    cpu.regs.PC -= cpu.fetched_data;
                }
                break;
            default:
                printf("unimplemented jmp condition\n");
                assert(0);
                break;
        }
        break;
    default:
        printf("unimplemented execute %d\n", cpu.curr_ins->kind);
        assert(0);
        break;
    }
}

void cpu_init()
{
    cpu.regs.PC = 0x20c; //0x150;
}

bool cpu_step() {
    if(!cpu.halt) {
        uint16_t pc = cpu.regs.PC;
        fetch_ins();
        fetch_data();
        printf("0x%04X: 0x%02X %s\n", pc, cpu.curr_op, cpu.curr_ins->str);
        execute();
        return true;
    }
    return false;
}

int main()
{
    uint8_t buffer[128];
    int rom_size;
    FILE *fp = fopen("Tetris.gb", "r");

    fseek(fp, 0, SEEK_END);
    int f_size = ftell(fp);

    rewind(fp);

    fseek(fp, 0x134, SEEK_SET);
    fread(buffer, 16, 1, fp);

    printf("Title: %s\n", buffer);

    fseek(fp, 0x148, SEEK_SET);
    fread(buffer, 1, 1, fp);
    int val = buffer[0];
    rom_size = 32 * 1024 * (1 << val);
    assert(f_size == rom_size);
    printf("ROM SIZE: %d\n", rom_size);
    printf("=========\n");
    rewind(fp);

    // Read Entire ROM
    rom_buffer = (uint8_t *) malloc(rom_size);
    fread(rom_buffer, rom_size, 1, fp);

    cpu_init();
    while (1)
    {
        if(!cpu_step()) {
            break;
        }
    }

    // regs.PC = 0x150;
    // int i;
    // uint8_t curr_op;
    // uint8_t byte1, byte2;

    // while (1) {
    //     printf("0x%x : ", regs.PC);
    //     curr_op = bus_read(regs.PC);
    //     switch (curr_op)
    //     {
    //     case 0x00:
    //         printf("NOP");
    //         regs.PC++;
    //         break;
        
    //     case 0x05:
    //         printf("DEC B");
    //         regs.B--;
    //         regs.PC++;
    //         break;

    //     case 0xc3:
    //         printf("JP ");
    //         byte1 = bus_read(regs.PC + 1);
    //         byte2 = bus_read(regs.PC + 2);
    //         regs.PC = (byte2 << 8) | byte1;
    //         printf("0x%x", regs.PC);
    //         break;

    //     case 0xaf:
    //         printf("XOR A, A");
    //         regs.A ^= regs.A;
    //         regs.PC++;
    //         break;

    //     case 0x21:
    //         printf("LD HL, ");
    //         byte1 = bus_read(regs.PC + 1);
    //         byte2 = bus_read(regs.PC + 2);
    //         regs.H = byte2;
    //         regs.L = byte1;
    //         printf("0x%x%x", byte2, byte1);
    //         regs.PC += 3;
    //         break;
        
    //     case 0x20:
    //         printf("JR NZ, ");
    //         byte1 = bus_read(regs.PC + 1);
    //         printf("0x%x", byte1);

    //         assert(0);
        
    //     case 0x06:
    //         printf("LD B, ");
    //         byte1 = bus_read(regs.PC + 1);
    //         regs.B = byte1;
    //         printf("0x%x", byte1);
    //         regs.PC += 2;
    //         break;

    //     case 0x0e:
    //         printf("LD C, ");
    //         byte1 = bus_read(regs.PC + 1);
    //         regs.C = byte1;
    //         printf("0x%x", byte1);
    //         regs.PC += 2;
    //         break;

    //     case 0x32:
    //         printf("LD (HL-), A");
    //         bus_write(regs.H << 8 | regs.L, regs.A);
    //         regs.PC++;
    //         break;
    //     default:
    //         printf("\ncurrent op %x\n", curr_op);
    //         assert(0 && "Unimplemented OP CODE");
    //         break;
    //     }
    //     printf("\n");
    // }

    fclose(fp);
    return 0;
}

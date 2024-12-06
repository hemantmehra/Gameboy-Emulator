#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "instruction.h"

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

Instruction instructions[0x100] = {
    [0x00] = {IN_NOP, AM_IMP},
    [0x05] = {IN_DEC, AM_R, REG_B},
    [0x06] = {IN_LD, AM_R_D8, REG_B},
    [0x0e] = {IN_LD, AM_R_D8, REG_C},
    [0xaf] = {IN_XOR, AM_R, REG_A},
    [0xc3] = {IN_JP, AM_D16}
};

// Z N H C
// 3 2 1 0
// enum Flags {
//     F_C,
//     F_H,
//     F_N,
//     F_Z,
// };

// void setF(enum Flags f)
// {
//     regs.F |= (1 << f); 
// }


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

uint16_t cpu_read_reg(reg_type rt)
{
    switch (rt)
    {
    case REG_A: return cpu.regs.A;
    case REG_B: return cpu.regs.B;
    case REG_C: return cpu.regs.C;

    default:
        printf("register unimplemented\n");
        assert(0);
        break;
    }
}

void fetch_ins()
{
    cpu.curr_op = bus_read(cpu.regs.PC++);
    cpu.curr_ins = &instructions[cpu.curr_op];
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
        return;
    }
    default:
        break;
    }
}

void execute()
{

}

void cpu_init()
{
    cpu.regs.PC = 0x150;
}

bool cpu_step() {
    if(!cpu.halt) {
        fetch_ins();
        fetch_data();
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

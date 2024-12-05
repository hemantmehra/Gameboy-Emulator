#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
    uint8_t A; uint8_t F;
    uint8_t B; uint8_t C;
    uint8_t D; uint8_t E;
    uint8_t H; uint8_t L;
    uint16_t PC;
    uint16_t SP;
} Registers;

int main()
{
    Registers regs;
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
    uint8_t *rom_buffer = (uint8_t *) malloc(rom_size);
    fread(rom_buffer, rom_size, 1, fp);

    regs.PC = 0x150;
    int i;
    uint8_t curr_op;
    uint8_t byte1, byte2;

    while (1) {
        printf("0x%x : ", regs.PC);
        curr_op = rom_buffer[regs.PC];
        switch (curr_op)
        {
        case 0x0:
            assert(0);
            break;

        case 0xc3:
            printf("JP ");
            byte1 = rom_buffer[regs.PC + 1];
            byte2 = rom_buffer[regs.PC + 2];
            regs.PC = (byte2 << 8) | byte1;
            printf("0x%x", regs.PC);
            break;

        case 0xaf:
            printf("XOR A, A");
            regs.PC += 1;
            break;

        case 0x21:
            printf("LD HL, ");
            byte1 = rom_buffer[regs.PC + 1];
            byte2 = rom_buffer[regs.PC + 2];
            i = (byte2 << 8) | byte1;
            regs.H = byte2;
            regs.L = byte1;
            printf("0x%x%x", byte2, byte1);
            regs.PC += 3;
            break;
        
        case 0x06:
            printf("LD B, ");
            byte1 = rom_buffer[regs.PC + 1];
            regs.B = byte1;
            printf("0x%x", byte1);
            regs.PC += 2;
            break;

        case 0x0e:
            printf("LD C, ");
            byte1 = rom_buffer[regs.PC + 1];
            regs.C = byte1;
            printf("0x%x", byte1);
            regs.PC += 2;
            break;

        default:
            printf("\ncurrent op %x\n", curr_op);
            assert(0 && "Unimplemented OP CODE");
            break;
        }
        printf("\n");
    }

    fclose(fp);
    return 0;
}

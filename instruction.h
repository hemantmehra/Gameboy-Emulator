#pragma once

#include <stdint.h>

typedef enum {
    REG_A,
    REG_B,
    REG_C
} reg_type;

typedef enum {
    AM_IMP,
    AM_R,
    AM_R_R,
    AM_R_D8,
    AM_D16
} addr_mod;

typedef enum {
    IN_INVALID,
    IN_NOP,
    IN_DEC,
    IN_JP,
    IN_XOR,
    IN_LD
} ins_kind;

typedef struct {
    ins_kind kind;
    addr_mod mode;
    reg_type reg1;
    reg_type reg2;
    uint8_t param;
} Instruction;
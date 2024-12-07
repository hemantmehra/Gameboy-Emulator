#pragma once

#include <stdint.h>

typedef enum {
    REG_A,
    REG_B,
    REG_C,
    REG_HL
} reg_type;

typedef enum {
    AM_IMP,
    AM_R,
    AM_R_R,
    AM_R_D8,
    AM_R_D16,
    AM_D16,
    AM_MR_R,
    AM_HLI_R,
    AM_HLD_R,
    AM_D8,
    AM_HL_SPR
} addr_mod;

typedef enum {
    IN_INVALID,
    IN_NOP,
    IN_DEC,
    IN_JP,
    IN_XOR,
    IN_LD
} ins_kind;

typedef enum {
    CT_NONE, CT_NZ, CT_Z, CT_NC, CT_C
} cond_type;

typedef struct {
    ins_kind kind;
    char *str;
    addr_mod mode;
    reg_type reg1;
    reg_type reg2;
    cond_type cond;
    uint8_t param;
} Instruction;

#pragma once

// http://www.c-jump.com/CIS77/CPU/x86/lecture.html
// http://www.c-jump.com/CIS77/CPU/x86/X77_0060_mod_reg_r_m_byte.htm
// http://tnovelli.net/ref/opx86.html
// http://sparksandflames.com/files/x86InstructionChart.html

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#pragma region Options
typedef enum { MODE_16 = 1, MODE_32 = 2, MODE_64 = 3 } bit_mode_e;
volatile bit_mode_e bits_mode;

#define BITS_MODE   bits_mode   // thread-safe implementation later
#define WORD_SIZE ((int)bits_mode << 1)
#pragma endregion

#if !NDEBUG
#define DEBUG 1
#endif

#if DEBUG == 1
#define STRINGBUILD(x, y) (x##y)
#define invalid_err(txt, ...) fprintf(stderr, STRINGBUILD("Invalid decoding: ",txt), __VA_ARGS__)
#else
#define invalid_err(txt, ...)
#endif

#define MAX_PREFIXES        0x04
#define MAX_OPCODES         0x03
#define MAX_OPERANDS        0x02
#define MAX_EXT_OPERANDS    0x0f

#pragma region Decoding flags
#define OPER_WORD        0x01    // ? word : byte
#define OPER_DIRECTION   0x02    // ? dest, src : src, dest
#define OPER_SIGNEXT     0x02    // ? imm8->sx : imm8->zx
#define OPER_ACCUM       0x04    // ? reg2 = accumulator : normal (no direction)

#define OPER_DEST        0x00    // dest first because single-op xrm decoding
#define OPER_SRC         0x01    // check flag for SINGLE before using

// uint16_t fProperties values:
#define OPF_NOP         0x0000  // no operands affected (e.g. nop, daa)
#define OPF_SINGLEOP    0x0001  // only one operand is used
#define OPF_DOUBLEOP    0x0002  // normal op xx, xx is used
#define OPF_MANYOP      0x0003  // triple+ operand (ie mul xx,xx,x, popad)
#define OPF_NUMOPS(x)   (x & 3) // use

#define OPF_MODRM       0x0004  // modrm byte is present
#define OPF_SIB         0x0008  // sib byte is present

#define OPF_IMMBYTE     0x0010
#define OPF_IMMWORD     0x0020
#define OPF_IMM         (OPF_IMMBYTE | OPF_IMMWORD)

#define OPF_DISPBYTE    0x0040
#define OPF_DISPWORD    0x0080
#define OPF_DISP        (OPF_DISPBYTE | OPF_DISPWORD)

#define OPF_BYTESIZE    0x0100  // operands are both byte sized
#define OPF_DIRECTION   0x0200  // ? reg1, reg2 (direction reversed)
                                // : reg2, reg1
#define OPF_EXTENDED    0x0400  // 2 opcodes present
#pragma endregion

#pragma region Operand defines
#define OT_RVAL     0x01    // .val contains a reg index
#define OT_LVAL     0x02    // .val contains an immediate value
#define OT_PTR      0x80    // .val is a ptr [val]

#define RT_GPR      0x01
#define RT_DBG      0x02
#define RT_CTRL     0x04
#define RT_MMX      0x08
#define RT_SSE      0x10
#define RT_3DNOW    0x20

#pragma region Reg Codes
#define GPR_ACCUM       0x00    // [e/r]ax
#define GPR_COUNT       0x01    // [e/r]cx
#define GPR_DATA        0x02    // [e/r]dx
#define GPR_BASE        0x03    // [e/r]bx
#define GPR_STACK       0x04    // [e/r]sp
#define GPR_STACKBASE   0x05    // [e/r]bp
#define GPR_SOURCE      0x06    // [e/r]si
#define GPR_DEST        0x07    // [e/r]di

#define GPR_LO_TO_HI(gpr) (gpr | 4)
#define GPR_HI_TO_LO(gpr) (gpr & ~4)

#define GPR_AL      0x00
#define GPR_CL      0x01
#define GPR_DL      0x02
#define GPR_BL      0x03
#define GPR_AH      (GPR_AL | 4)
#define GPR_CH      (GPR_CL | 4)
#define GPR_DH      (GPR_DL | 4)
#define GPR_BH      (GPR_BL | 4)

#define SREG_ES     0x00
#define SREG_CS     0x01
#define SREG_SS     0x02
#define SREG_DS     0x03
#define SREG_FS     0x04
#define SREG_GS     0x05
#pragma endregion

#pragma region Prefixes
    #define PRE_OPER_OVERRIDE	0x66
    #define PRE_ADDR_OVERRIDE	0x67
    #define PRE_ES				0x2e	// never gonna happen in 32
    #define PRE_CS				0x3e
    #define PRE_SS				0x26	// never gonna happen in 32
    #define PRE_DS				0x36	// never gonna happen in 32
    #define PRE_FS				0x64
    #define PRE_GS				0x65
    #define PRE_REP				0xf3
    #define PRE_REPE			PRE_REP
    #define PRE_REPNE			0xf2
    #define PRE_LOCK			0xf0

    #define F_PRE_OPSIZ	        0x0001    // 16 bit
    #define F_PRE_ADRSIZ	    0x0002    // 16 bit
    #define F_PRE_SEG           0x0004    // pre_seg + sreg_xx
    #define F_PRE_ES			0x0004    // selectors hardly used
    #define F_PRE_CS			0x0008    //
    #define F_PRE_SS			0x0010    //
    #define F_PRE_DS			0x0020    //
    #define F_PRE_FS			0x0040    //
    #define F_PRE_GS			0x0080    //
    #define F_PRE_REP			0x0100    // repeat
    #define F_PRE_REPE			PRE_REP   // |
    #define F_PRE_REPNE			0x0200    // v
    #define F_PRE_LOCK			0x0400    //
    #define F_PRE_EXPANSION      0x0800    // extended opcode is first (not in prefix table)

    #define F_PRE_16            (F_PRE_OPSIZ | F_PRE_ADRSIZ)

    #pragma endregion
#pragma endregion

typedef struct {
    // x += 1 << (.Xxx & 2) * ((.Xxx >> 2) ? -1 : 1) = change (+=max 8)
    uint32_t    dEax    : 3,    // 0
                dEcx    : 3,    // 3
                dEdx    : 3,    // 6
                dEbx    : 3,    // 9
                dEsp    : 5,    // 12 pushad/pushfd (number)
                dEbp    : 3,    // 17
                dEsi    : 3,    // 20
                dEdi    : 3,    // 23
#define AFF_SEGREG 26
                dEs     : 1,    // 26
                dCs     : 1,    // 27
                dSs     : 1,    // 28
                dDs     : 1,    // 29
                dFs     : 1,    // 30
                dGs     : 1;    // 31
} instr_register_affected_t;

typedef struct {    // modrm_byte = *(instr->ip + instr_outline->iModrm)
    // prefix is always 0th byte
    // couldn't pack into uint16_t (was one over), so nibbles it is
    uint32_t iOpcode    : 4,    // 4 max prefix
    // if 0, these bytes are not present
            iModrm      : 4,    // 4 prefix + 2 opcode
            iSib        : 4,    // 4 prefix + 2 opcode + 1 modrm
            iDisp       : 4,    // 4 prefix + 2 opcode + 1 modrm + 1 sib
            iImm        : 4;    // 4 prefix + 2 opcode + 1 modrm + 1 sib + 4 disp
} instr_outline_t;

typedef struct {
    uint8_t     type;   // OT_xxx flag
    uint8_t     regtype;// set if type == OT_RVAL
    uint8_t     reg;    // reg code
    uint8_t     size;   // number of bytes
    union {
        uint8_t val8;
        uint16_t val16;
        uint32_t val32;
        uint64_t val64;
    } val;              // imm/disp value
} operand_t;

typedef struct {
    void           *next_instr,
                   *prev_instr;
    unsigned char*  ip;                     // location of decoded instr
    uint16_t        fPrefix;                // prefix flags
    uint16_t        fProperties;            // extended instr information
    uint8_t         nOpcodes;               // number of opcode bytes
    uint8_t         nPrefixes;              // number of prefix bytes
    uint8_t         cbOpcode;               // total opcode size
    instr_outline_t outline;
    instr_register_affected_t affected;
    operand_t       operand[MAX_OPERANDS];  // decoded operands
} instr_t;

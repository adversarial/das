
#include "das.h"

int decompose(const unsigned char* pDis, instr_t* instr) {

    instr->ip = pDis;

instr_loop:
    opcode = *pDis++;
    if (opcode < 0100) {
        // group 1 opcode
        if (opcode == 0x0f) {
            // extended prefix
            instr->fProperties |= OPF_EXTENDED;
            ++instr->cbOpcode;
        }
        else if ((opcode & 7) < 6) { 
            // logical math instruction
            instr->fProperties |= OPF_DOUBLEOP;

            if (opcode & OPER_ACCUM) {
                // reg2 = accumulator
                instr->fProperties |= (opcode & OPER_WORD) ? OPF_IMMWORD : OPF_IMMBYTE | OPF_BYTESIZE;
                instr->outline.iImm = (instr->cbOpcode += (opcode & OPER_WORD) ? WORD_SIZE : 1);
            } else { // if !(opcode & OPF_ACCUM)
                // op reg2, reg1
                instr->fProperties |= OPF_MODRM 
                                   | (opcode & OPER_WORD) ? 0 : OPF_BYTESIZE 
                                   | (opcode & OPER_DIRECTION) ? 0 : OPF_DIRECTION;
                instr->outline.iModrm = instr->cbOpcode++;
            }
        } else if (opcode < 040) {
            if (opcode & 01) {
                // pop segreg
                uint32_t* affected = (uint32_t*)&instr->affected;
                *affected |= 1 << (AFF_SEGREG + ((opcode & 0x18) >> 3));
                instr->affected.dEsp = -4;
            }
        } else if (opcode & 01) {
            // bcd instruction
            instr->affected.dEax = 1;   // magic happens
            instr->fProperties |= OPF_NOP | OPF_BYTESIZE;
            /*
            if (opcode & 0x10)
                // packed bcd
            else
                // unpacked bcd
            */
        } else { // if !(opcode & 01)
            // segment override prefix
            instr->fPrefix |= F_PRE_SEG + ((opcode & 0x18) >> 3);
            ++instr->cbOpcode;
            goto instr_loop;
        }
    }
    return 0;
}

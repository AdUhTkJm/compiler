#pragma once
#include "ast.h"

enum ir_type {
    I_IMM,          // immediate value
    I_ADD,          // add
    I_SUB,          // sub
    I_IMUL,         // imul
    I_IDIV,         // cqo; idiv; rax
    I_MOD,          // cqo; idiv; rdx
    I_RET,          // mov rax, {}; epilogue
};

// Note: register is a keyword

class reg {
    static int cnt;
public:
    // (virtual) index of the register
    int ind;

    // These properties are used when tidying registers.

    // real index of the register
    int real;
    // the last place where the register is used
    int last;
    // the first place where the register comes into existence
    int first;

    reg();
};

struct ir {
    ir_type ty;
    // operands of the instruction
    reg *a0, *a1, *a2;
    // immediate value
    int imm;

    ir(ir_type ty, int imm, reg* a0);
    ir(ir_type ty, reg* a0, reg* a1=nullptr, reg* a2=nullptr);
};

std::vector<ir> generate(node*);
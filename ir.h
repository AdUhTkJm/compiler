#pragma once
#include "ast.h"
#include <map>

enum ir_type {
    I_IMM,          // immediate value
    I_ADD,          // add
    I_SUB,          // sub
    I_IMUL,         // imul
    I_IDIV,         // cqo; idiv; rax
    I_MOD,          // cqo; idiv; rdx
    I_RET,          // mov rax, {}; epilogue
    I_STORE,        // mov [{}], ...
    I_LOCALREF,     // lea {}, [rbp-offset];
    I_GLOBALREF,    // lea {}, VARNAME;
    I_LOAD,         // mov {}, [...]
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
    // variable involved
    var* v;
    // size of LOAD/STORE
    int sz;

    ir(ir_type ty, int imm, reg* a0);
    ir(ir_type ty, reg* a0, reg* a1=nullptr, reg* a2=nullptr);
    ir(ir_type ty, reg* a0, var* v);
    ir(ir_type ty, reg* a0, reg* a1, int sz);
};

std::map<func*, std::vector<ir>> generate();
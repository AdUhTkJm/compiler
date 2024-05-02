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
    I_RET,          // mov rax, {}; jmp
    I_STORE,        // mov [{}], ...
    I_LOCALREF,     // lea {}, [rbp-offset];
    I_GLOBALREF,    // lea {}, VARNAME;
    I_LOAD,         // mov {}, [...]
    I_CALL,         // call
    I_IF,           // cmp {}, 0; jne
    I_WHILE,        // cmp {}, 0; jne
    I_FOR,          // cmp {}, 0; jne
    I_GE,           // setg
    I_LE,           // setl
    I_LEQ,          // setle
    I_GEQ,          // setge
    I_NEQ,          // setne
    I_EQ,           // sete
    I_RAW,          // (raw assembly)
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
    // for I_IMM, immediate value
    // for I_JMP, the number of label
    int imm;
    // variable involved
    var* v;
    // size of LOAD/STORE
    int sz;
    // parameters of function call
    std::vector<reg*> params;
    // name of function call
    std::string name;

    ir(ir_type ty, int imm, reg* a0);
    ir(ir_type ty, reg* a0=nullptr, reg* a1=nullptr, reg* a2=nullptr);
    ir(ir_type ty, reg* a0, var* v);
    ir(ir_type ty, reg* a0, reg* a1, int sz);
    ir(std::string);
};

std::map<func*, std::vector<ir*>> generate();
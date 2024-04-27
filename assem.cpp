#define FMT_HEADER_ONLY

#include "assem.h"
#include "fmt/format.h" // workaround of C++20
#include <iostream>
#include <algorithm>

using fmt::format;

// There are 7 registers for x86 that we can use.
// They are r10 to r15 and rbx.
reg* used[7] = { 0 };
const char* regs[] = { "r10", "r11", "r12", "r13", "r14", "r15", "rbx" };

void assign(reg* a) {
    if (!a)
        return;

    bool found = false;
    
    for (int i = 0; i < 7; i++) {
        if (used[i] && used[i]->last > a->first)
            continue;
        used[i] = a;
        a->real = i;
        found = true;
        break;
    }
    
    if (!found)
        std::cout << "Not found!!" << std::endl;
}

void tidy_register(std::vector<ir>& irs) {
    std::vector<reg*> regs;
    // instruction counter
    // starts at 1, so that !first will not fail
    int ic = 1;

    for (int i = 0; i < irs.size(); i++, ic++) {
        auto x = irs[i];

        if (x.a0 && !x.a0->first) {
            regs.push_back(x.a0);
            x.a0->first = ic;
        }
        
        // a1 and a2 never gets newly defined
        if (x.a0)
            x.a0->last = std::max(x.a0->last, ic);
        if (x.a1)
            x.a1->last = std::max(x.a1->last, ic);
        if (x.a2)
            x.a2->last = std::max(x.a2->last, ic);
    }

    std::for_each(regs.begin(), regs.end(), assign);
}

void assemble(std::ostream& os, std::vector<ir>& irs) {
    os <<
    ".intel_syntax noprefix\n"
    ".globl main\n"
    "main:\n"
    "\tpush rbp\n"
    "\tmov rbp, rsp\n";
    tidy_register(irs);
    for (auto x : irs) {
        auto r0 = regs[x.a0 ? x.a0->real : 0];
        auto r1 = regs[x.a1 ? x.a1->real : 0];
        auto r2 = regs[x.a2 ? x.a2->real : 0];
        
        switch (x.ty) {
        case I_IMM:
            os << format("\tmov {}, {}\n", r0, x.imm);
            break;
        case I_ADD:
            os << format("\tadd {}, {}\n", r0, r1);
            break;
        case I_SUB:
            os << format("\tsub {}, {}\n", r0, r1);
            break;
        case I_IMUL:
            os << format("\tmov rax, {}\n", r0)
            << format("\timul {}\n", r1)
            << format("\tmov {}, rax\n", r0);
            break;
        case I_IDIV:
            os << format("\tmov rax, {}\n", r0)
            << "\tcqo\n"
            << format("\tidiv {}\n", r1)
            << format("\tmov {}, rax\n", r0);
            break;
        case I_MOD:
            os << format("\tmov rax, {}\n", r0)
            << "\tcqo\n"
            << format("\tidiv {}\n", r1)
            << format("\tmov {}, rdx\n", r0);
            break;
        case I_RET:
            os << format("\tmov rax, {}\n", r0) << 
            "\tmov rsp, rbp\n"
            "\tpop rbp\n"
            "\tret\n";
        }
    }
}
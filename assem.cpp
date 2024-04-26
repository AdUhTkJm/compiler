#define FMT_HEADER_ONLY

#include "assem.h"
#include "fmt/format.h" // workaround of C++20
#include <iostream>
#include <algorithm>

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
    int ic;

    for (int i = 0; i < irs.size(); i++, ic++) {
        auto x = irs[i];

        if (x.a0 && !x.a0->first)
            regs.push_back(x.a0);
        
        // a1 and a2 never gets newly defined
        if (x.a1)
            x.a1->last = std::max(x.a1->last, ic);
        if (x.a2)
            x.a2->last = std::max(x.a2->last, ic);
    }

    std::for_each(regs.begin(), regs.end(), assign);
}

void emit(std::ostream& os, const char* str, reg* r1, reg* r2) {
    os << "\t" << fmt::format(str, regs[r1->real], regs[r2->real]) << "\n";
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
        switch (x.ty) {
        case I_IMM:
            os << fmt::format("\tmov {}, {}\n", regs[x.a0->real], x.imm);
            break;
        case I_ADD:
            emit(os, "add {}, {}", x.a0, x.a1);
            break;
        case I_SUB:
            emit(os, "sub {}, {}", x.a0, x.a1);
            break;
        }
    }
    os <<
    "\tmov rsp, rbp\n"
    "\tpop rbp\n"
    "\tret\n";
}
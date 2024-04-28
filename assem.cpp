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

// Round val up to the smallest integer that is divisible by x
// and larger than val.
int round_up(int val, int x) {
    return ceil(1.0 * val / x) * x;
}

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

void assemble_var(std::ostream& os) {
    for (auto x : global->vars) {
        // All explicitly initialised variables should go into data segment
        // But we do not support explicit initialisation yet
        // So we put everything in .bss, which is zero-initialised
        os << ".bss\n";
        os << format("{}:\n\t.zero {}\n", x->name, x->ty.sz);
    }
}

std::string reg_ofsize(std::string x, int sz) {
    if (x == "rbx") {
        if (sz == 8)
            return "rbx";
        if (sz == 4)
            return "ebx";
        if (sz == 2)
            return "bx";
        if (sz == 1)
            return "bl";
    }
    if (sz == 8)
        return x;
    if (sz == 4)
        return x + "d";
    if (sz == 2)
        return x + "w";
    if (sz == 1)
        return x + "b";
    
    return "UNSUPPORTED SIZE " + std::to_string(sz);
}

void assemble_func(std::ostream& os, func* f, std::vector<ir>& irs) {
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
            os << format("\tmov rax, {}\n", r0)
            << format("\tjmp .Lfunc_end_{}\n", f->name);
            break;
        case I_LOCALREF:
            os << format("\tlea {}, [rbp{}]\n", r0, x.v->offset);
            break;
        case I_GLOBALREF:
            os << format("\tlea {}, {}\n", r0, x.v->name);
            break;
        case I_STORE:
            os << format("\tmov [{}], {}\n", r0, reg_ofsize(r1, x.sz));
            break;
        case I_LOAD:
            os << format("\tmov {}, [{}]\n", reg_ofsize(r0, x.sz), r1);
            break;
        default:
            throw x.ty;
        }
    }
}

void assemble(std::ostream& os, decltype(generate())& irs) {
    os << ".intel_syntax noprefix\n";

    for (auto [f, i] : irs) {
        int offset = 0;
        for (auto v : f->v->vars) {
            v->offset = -(offset += v->ty.sz);
        }
        os << format(".text\n.global {}\n{}:\n", f->name, f->name) <<
        "\tpush rbp\n"
        "\tmov rbp, rsp\n";
        
        // rsp must get aligned to a multiple to 16 for "call" instruction to work
        os << format("\tsub rsp, {}\n", round_up(offset, 16));
        
        // Preserve registers
        for (int i = 12; i <= 15; i++)
            os << format("\tpush r{}\n", i);

        assemble_func(os, f, i);

        os << format(".Lfunc_end_{}:\n", f->name);
        
        for (int i = 12; i <= 15; i++)
            os << format("\tpop r{}\n", i);
        
        os <<
        "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n";
    }
}
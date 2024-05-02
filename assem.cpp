#define FMT_HEADER_ONLY

#include "assem.h"
#include "fmt/format.h" // workaround of C++20
#include <iostream>
#include <algorithm>
#define MAPPED std::make_pair

using fmt::format;

// There are 7 registers for x86 that we can use.
// They are r10 to r15 and rbx.
// Other registers are unused because they are of different uses (eg. function arguments)
reg* used[7] = { 0 };
const char* regs[] = { "r10", "r11", "r12", "r13", "r14", "r15", "rbx" };

std::map<ir_type, std::string> cmpmap {
    MAPPED(I_LE, "setl"),
    MAPPED(I_GE, "setg"),
    MAPPED(I_LEQ, "setle"),
    MAPPED(I_GEQ, "setge"),
    MAPPED(I_EQ, "sete"),
    MAPPED(I_NEQ, "setne"),
};

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
    
    // TODO: consider times when we need more than 7 registers.
    if (!found)
        std::cout << "Not found!!" << std::endl;
}

void tidy_register(std::vector<ir*>& irs) {
    std::vector<reg*> regs;
    // instruction counter
    // starts at 1, so that !first will not fail
    int ic = 1;

    memset(used, 0, sizeof used);

    for (int i = 0; i < irs.size(); i++, ic++) {
        auto x = irs[i];

        if (x->a0 && !x->a0->first) {
            regs.push_back(x->a0);
            x->a0->first = ic;
        }
        
        // a1 and a2 never gets newly defined
        if (x->a0)
            x->a0->last = std::max(x->a0->last, ic);
        if (x->a1)
            x->a1->last = std::max(x->a1->last, ic);
        if (x->a2)
            x->a2->last = std::max(x->a2->last, ic);
        if (x->params.size())
            for (auto r : x->params)
                r->last = std::max(r->last, ic);
    }

    std::for_each(regs.begin(), regs.end(), assign);
}

void assemble_var(std::ostream& os) {
    // All explicitly initialised variables should go into data segment
    // But we do not support explicit initialisation yet
    // So we put everything in .bss, which is zero-initialised  
    os << "section .bss\n";
    for (auto x : global->vars) {  
        os << format("{} resb {}\n", x->name, x->ty.sz);
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

// We assume i < 6.
std::string reg_arg(int i, int sz = 8) {
    if (sz > 8 || sz & (sz - 1))
        return "UNSUPPORTED SIZE " + std::to_string(sz);

    static std::string rs8[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
    static std::string rs4[] = { "edi", "esi", "edx", "ecx", "r8d", "r9d" };
    static std::string rs2[] = { "di", "si", "dx", "cx", "r8w", "r9w" };
    static std::string rs1[] = { "dil", "sil", "dl", "cl", "r8b", "r9b" };
    return (sz == 8 ? rs8 : sz == 4 ? rs4 : sz == 2 ? rs2 : rs1)[i];
}

void assemble_func(std::ostream& os, func* f, std::vector<ir*>& irs) {
    tidy_register(irs);
    for (auto x : irs) {
        auto r0 = regs[x->a0 ? x->a0->real : 0];
        auto r1 = regs[x->a1 ? x->a1->real : 0];
        auto r2 = regs[x->a2 ? x->a2->real : 0];
        
        switch (x->ty) {
        case I_IMM:
            os << format("\tmov {}, {}\n", r0, x->imm);
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
        case I_LE:
        case I_GE:
        case I_LEQ:
        case I_GEQ:
        case I_NEQ:
        case I_EQ:
            os << format("\tcmp {}, {}\n", r0, r1)
            << format("\t{} {}\n", cmpmap[x->ty], reg_ofsize(r0, 1))
            << format("\tmovzx {}, {}\n", r0, reg_ofsize(r0, 1));
            break;
        case I_RET:
            os << format("\tmov rax, {}\n", r0)
            << format("\tjmp .Lfunc_end_{}\n", f->name);
            break;
        case I_LOCALREF: {
            std::string off = std::to_string(x->v->offset);
            os << format("\tlea {}, [rbp{}]\n", r0, x->v->offset > 0 ? "+" + off : off);
            break;
        }
        case I_GLOBALREF:
            os << format("\tlea {}, {}\n", r0, x->v->name);
            break;
        case I_STORE:
            os << format("\tmov [{}], {}\n", r0, reg_ofsize(r1, x->sz));
            break;
        case I_LOAD:
            os << format("\tmov {}, [{}]\n", reg_ofsize(r0, x->sz), r1);
            break;
        case I_CALL: {
            int sz = x->params.size();
            for (int i = 0; i < std::min(6, sz); i++)
                os << format("\tmov {}, {}\n", reg_arg(i), regs[x->params[i]->real]);
            
            // Caller-preserved registers
            os << "\tpush r10\n\tpush r11\n";

            // Push excessive arguments to the stack
            for (int i = std::min(6, sz); i < sz; i++)
                os << format("\tpush {}\n", regs[x->params[i]->real]);

            // main() shall return 0 if it doesn't explicitly return
            // otherwise unspecified return value is UB, so do whatever
            os << "\tmov rax, 0\n";

            os << format("\tcall {}\n", x->name);

            // Clear arguments pushed to the stack
            if (sz > 6)
                os << format("\tadd rsp, {}\n", (sz - 6) * 8);

            os << "\tpop r11\n\tpop r10\n";

            os << format("\tmov {}, rax\n", r0);

            break;
        }
        case I_IF:
            os << format("\tcmp {}, 0\n", r0)
            << format("\tje .Lif_{}_unhit\n", x->imm);
            break;    
        case I_WHILE:
            os << format("\tcmp {}, 0\n", r0)
            << format("\tje .Lwhile_{}_end\n", x->imm);
            break;
        case I_FOR:
            os << format("\tcmp {}, 0\n", r0)
            << format("\tje .Lfor_{}_end\n", x->imm);
            break;
        case I_RAW:
            if (x->name[0] != '.')
                os << "\t";
            os << x->name << "\n";
            break;
        default:
            throw x->ty;
        }
    }
}

void assemble(std::ostream& os, decltype(generate())& irs) {
    assemble_var(os);
    os << "\n";

    for (auto [f, i] : irs) {
        os << format("section .text\nglobal {}\n{}:\n", f->name, f->name) <<
        "\tpush rbp\n"
        "\tmov rbp, rsp\n";

        int offset = 0, off_param = 8;
        for (auto v : f->v->vars) {
            if (v->is_param)
                continue;
            v->offset = -(offset += v->ty.sz);
        }
        for (int i = 0; i < f->params.size(); i++) {
            var* v = f->params[i];
            if (i < 6)
                v->offset = -(offset += v->ty.sz);
            else
                v->offset = off_param += 8; // Every push grows stack by 8 bytes
        }
        
        // rsp must get aligned to a multiple to 16 for "call" instruction to work
        os << format("\tsub rsp, {}\n", round_up(offset, 16));
        
        // Preserve registers by calling convention
        for (int i = 12; i <= 15; i++)
            os << format("\tpush r{}\n", i);
        os << format("\tpush rbx\n");

        // Copy arguments to stack
        for (int i = 0; i < f->params.size() & i < 6; i++) {
            var* v = f->params[i];
            os << format("\tmov [rbp{}], {}\n", v->offset, reg_arg(i, v->ty.sz));
        }

        assemble_func(os, f, i);

        os << format(".Lfunc_end_{}:\n", f->name);
        
        os << format("\tpop rbx\n");
        for (int i = 15; i >= 12; i--)
            os << format("\tpop r{}\n", i);
        
        os <<
        "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n";
    }
}
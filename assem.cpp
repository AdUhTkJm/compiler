#define FMT_HEADER_ONLY

#include "assem.h"
#include "fmt/format.h" // workaround of C++20
#include <iostream>
#include <algorithm>
#define MAPPED std::make_pair

using fmt::format;

// Other registers are unused because they are of different uses (eg. function arguments)
// Perhaps fix this later.
const char* regs[] = { "r10", "r11", "r12", "r13", "r14", "r15", "rbx", "rdi" };
const int rsz = sizeof(regs) / sizeof(const char*);
reg* used[rsz] = { 0 };

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

bool assign(func* f, reg* a) {
    if (!a)
        return true;

    bool found = false;
    
    // We preserve the final register as a destination of spilling
    // All spilt registers store their values there
    for (int i = 0; i < rsz - 2; i++) {
        if (used[i] && used[i]->last > a->first)
            continue;
        used[i] = a;
        a->real = i;
        found = true;
        break;
    }
    
    return found;
}

// Returns how many registers we need to push & pop
int tidy_register(func* f, std::vector<ir*>& irs) {
    std::vector<reg*> rs;
    // instruction counter
    // starts at 1, so that !first will not fail
    int ic = 1;

    memset(used, 0, sizeof used);

    for (int i = 0; i < irs.size(); i++, ic++) {
        auto x = irs[i];

        // a1 never gets newly defined
        if (x->a0 && !x->a0->first) {
            rs.push_back(x->a0);
            x->a0->first = ic;
        }
        
        if (x->a0)
            x->a0->last = std::max(x->a0->last, ic);
        if (x->a1)
            x->a1->last = std::max(x->a1->last, ic);
        if (x->params.size())
            for (auto r : x->params)
                r->last = std::max(r->last, ic);
    }

    std::sort(rs.begin(), rs.end(), [](reg* a, reg* b) {
        return a->first < b->first;
    });
    for (auto r : rs) {
        if (assign(f, r))
            continue;
        
        var* v = new var;
        v->ty = type::ptr(new type(K_INT));
        f->v->push(v);

        r->spilt = true;
        r->dest = v;
        r->real = 0;
    }

    int max = 0;
    for (auto r : rs)
        max = std::max(max, r->real);
    return std::max(0, max - 1);
}

void assemble_var(std::ostream& os) {
    // All explicitly initialised variables should go into data segment
    os << "section .data\n";
    for (auto x : global->vars)
        if (x->is_strlit) {
            os << format("{}: db ", x->name);
            // To avoid things like, say, \n in strings
            // getting printed out as a new line rather than as "\\n"
            for (auto x : x->value)
                os << format("{}, ", (int) x);
            os << "0\n";
        }
    
    // The rest of everything is in .bss, which is zero-initialised  
    os << "section .bss\n";
    for (auto x : global->vars)
        if (!x->is_strlit)
            os << format("{} resb {}\n", x->name, x->ty->sz);
    
}

std::string sized(std::string x, int sz) {
    if (sz & (sz - 1) || sz > 8)
        return "UNSUPPORTED SIZE " + std::to_string(sz);

    if (x == "rbx")
        return sz == 8 ? "rbx" : sz == 4 ? "ebx" : sz == 2 ? "bx" : "bl";
    if (x == "rdi")
        return sz == 8 ? "rdi" : sz == 4 ? "ebx" : sz == 2 ? "di" : "dil";
    
    return x + (sz == 8 ? "" : sz == 4 ? "d" : sz == 2 ? "w" : "b");
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

bool spilt(ir* x) {
    return x->a0 && x->a0->spilt || x->a1 && x->a1->spilt;
}

void assemble_func(std::ostream& os, func* f, std::vector<ir*>& irs, bool reassigned = false) {
    for (auto x : irs) {
        auto r0 = regs[x->a0 ? x->a0->real : 0];
        auto r1 = regs[x->a1 ? x->a1->real : 0];

        // Allocate spilt registers
        // We do not do this in tidy_registers(), because
        // it might assign to the same register twice,
        // in difference instructions
        if (spilt(x) && !reassigned) {
            std::vector<ir*> sp;
            if (x->a0 && x->a0->spilt) {
                x->a0->real = rsz - 2;
                sp.push_back(new ir(I_SPILL_LOAD, regs[rsz - 2], x->a0->dest));
            }
            if (x->a1 && x->a1->spilt) {
                sp.push_back(new ir(I_SPILL_LOAD, regs[rsz - 1], x->a1->dest));
                if (x->a0 != x->a1)
                    x->a1->real = rsz - 1;
            }

            sp.push_back(x);

            if (x->a0 && x->a0->spilt)
                sp.push_back(new ir(I_SPILL_STORE, regs[rsz - 2], x->a0->dest));

            assemble_func(os, f, sp, true);
            continue;
        }
        
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
            << format("\t{} {}\n", cmpmap[x->ty], sized(r0, 1))
            << format("\tmovzx {}, {}\n", r0, sized(r0, 1));
            break;
        case I_RET:
            if (x->a0)
                os << format("\tmov rax, {}\n", r0);
            os << format("\tjmp .Lfunc_end_{}\n", f->name);
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
            os << format("\tmov [{}], {}\n", r0, sized(r1, x->sz));
            break;
        case I_LOAD:
            os << format("\tmov {}, [{}]\n", sized(r0, x->sz), r1);
            // In case the front bits of r0 has been used and not emptied
            if (x->sz != 8)
                os << format("\tmovsx {}, {}\n", r0, sized(r0, x->sz));
            break;
        case I_SPILL_STORE:
            os << format("\tmov [rbp{}], {}\n", x->v->offset, x->name);
            break;
        case I_SPILL_LOAD:
            os << format("\tmov {}, [rbp{}]\n", x->name, x->v->offset);
            break;
        case I_CALL: {
            int sz = x->params.size();
            for (int i = 0; i < std::min(6, sz); i++)
                os << format("\tmov {}, {}\n", reg_arg(i), regs[x->params[i]->real]);
            
            // Variadic function must pass excessive argument amount to %al
            os << format("\tmov al, {}\n", x->imm);
            
            // Caller-preserved registers
            os << "\tpush r10\n\tpush r11\n";

            // Push excessive arguments to the stack
            for (int i = std::min(6, sz); i < sz; i++)
                os << format("\tpush {}\n", regs[x->params[i]->real]);

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

    // List of callee-preserved registers we use
    static std::string hold[] = { "r12", "r13", "r14", "r15", "rbx" };

    for (auto& [f, i] : irs) {
        // Case: Declaration only
        if (i.empty()) {
            os << format("extern {}\n", f->name);
            continue;
        }

        // Tidy registers before anything happens,
        // since this changes the vector<ir*>
        // Also records how many registers we used that we have to preserve
        int amt = tidy_register(f, i);

        // Case: Definition
        os << format("section .text\nglobal {}\n{}:\n", f->name, f->name) <<
        "\tpush rbp\n"
        "\tmov rbp, rsp\n";

        // Assign an offset to all local variables
        int offset = 0, off_param = 8;
        for (auto v : f->v->vars) {
            if (v->is_param)
                continue;
            v->offset = -(offset += v->ty->sz);
        }
        for (int i = 0; i < f->params.size(); i++) {
            var* v = f->params[i];
            if (i < 6)
                v->offset = -(offset += v->ty->sz);
            else
                v->offset = off_param += 8; // Every push grows stack by 8 bytes
        }
        
        // rsp must get aligned to a multiple to 16 by calling convention
        // we pushed #amt registers, so rsp is decreased by 8 * amt,
        // which we also need to take into account
        int align = round_up(offset, 8);
        if ((align + amt * 8) % 16)
            align += 8;
        os << format("\tsub rsp, {}\n", align);
        
        // Preserve registers by calling convention
        for (int i = 0; i < amt; i++)
            os << format("\tpush {}\n", hold[i]);

        // Copy arguments to stack
        for (int i = 0; i < f->params.size() & i < 6; i++) {
            var* v = f->params[i];
            os << format("\tmov [rbp{}], {}\n", v->offset, reg_arg(i, v->ty->sz));
        }

        assemble_func(os, f, i);

        os << format(".Lfunc_end_{}:\n", f->name);
        
        for (int i = amt - 1; i >= 0; i--)
            os << format("\tpop {}\n", hold[i]);
        
        os <<
        "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n";
    }
}
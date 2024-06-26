#include "ir.h"
#include "fmt/format.h"
#include <map>
#define MAPPED std::make_pair

using fmt::format;

std::map<node_type, ir_type> opmap {
    MAPPED(N_PLUS, I_ADD),
    MAPPED(N_MINUS, I_SUB),
    MAPPED(N_MUL, I_IMUL),
    MAPPED(N_DIV, I_IDIV),
    MAPPED(N_MOD, I_MOD),
    MAPPED(N_LE, I_LE),
    MAPPED(N_LEQ, I_LEQ),
    MAPPED(N_GE, I_GE),
    MAPPED(N_GEQ, I_GEQ),
    MAPPED(N_NEQ, I_NEQ),
    MAPPED(N_EQ, I_EQ),
};

ir::ir(ir_type ty, int imm, reg* a0):
    ty(ty), imm(imm), a0(a0), a1(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1):
    ty(ty), a0(a0), a1(a1) {}

ir::ir(ir_type ty, reg* a0, var* v):
    ty(ty), a0(a0), v(v), a1(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, int sz):
    ty(ty), a0(a0), a1(a1), sz(sz) {}

ir::ir(ir_type ty, std::string name, var* v):
    ty(ty), name(name), v(v), a0(nullptr), a1(nullptr) {}

ir::ir(std::string str):
    ty(I_RAW), name(str), a0(nullptr), a1(nullptr) {}

int reg::cnt = 0;
reg::reg(): ind(cnt++), spilt(false), first(0), last(0), real(0) {}


// The result of generate()
std::vector<ir*> res;

template<class... T>
void push(T... args) {
    res.push_back(new ir(args...));
}

reg* gen_imm(node* x) {
    reg* a0 = new reg;
    push(I_IMM, x->val, a0);
    return a0;
}

reg* gen_expr(node*);
reg* gen_addr(node* x) {
    reg* a0 = new reg;

    // since I_STORE needs pointer after all
    if (x->ty == N_DEREF)
        return gen_expr(x->lhs);

    // here, x->ty == N_VARREF
    if (x->target->is_global)
        push(I_GLOBALREF, a0, x->target);
    else
        push(I_LOCALREF, a0, x->target);

    return a0;
}

reg* gen_expr(node* x) {
    reg *a0 = nullptr, *a1, *a2;

    static int if_cnt = 0;
    static int while_cnt = 0;
    static int for_cnt = 0;

    switch (x->ty) {
    case N_NUM:
        return gen_imm(x);
    case N_RET:
        if (x->lhs)
            a0 = gen_expr(x->lhs);

        push(I_RET, a0);
        return a0;
    case N_PLUS:
    case N_MINUS:
    case N_MUL:
    case N_DIV:
    case N_MOD:
    case N_EQ:
    case N_LEQ:
    case N_GEQ:
    case N_NEQ:
    case N_GE:
    case N_LE:
        a0 = gen_expr(x->lhs);
        a1 = gen_expr(x->rhs);
        
        push(opmap[x->ty], a0, a1);
        return a0;
    case N_VARREF:
        a0 = new reg;
        a1 = gen_addr(x);

        push(I_LOAD, a0, a1, x->cty->sz);
        return a0;
    case N_DEREF:
        a0 = new reg;
        a1 = gen_expr(x->lhs);

        push(I_LOAD, a0, a1, x->cty->sz);
        return a0;
    case N_ADDR:
        a0 = gen_addr(x->lhs);

        return a0;
    case N_ASSIGN:
        a0 = gen_addr(x->lhs);
        a1 = gen_expr(x->rhs);

        push(I_STORE, a0, a1, x->cty->sz);
        return a1; // !! We need value rather than address
    case N_BLOCK:      
        for (auto m : x->nodes)
            a0 = gen_expr(m);

        return a0;
    case N_FCALL: {
        a0 = new reg;

        ir* i = new ir(I_CALL, a0);
        for (auto m : x->nodes)
            i->params.push_back(gen_expr(m));
        i->name = x->name;
        i->imm = x->val;
        res.push_back(i);
        return a0;
    }
    case N_IF: {
        // Record current count
        // in case x->lhs contains another if clause
        int my_cnt = if_cnt++;
        a0 = gen_expr(x->cond);
        push(I_IF, my_cnt, a0);

        gen_expr(x->lhs);

        if (x->rhs) {
            push(format("jmp .Lif_{}_end", my_cnt));
            push(format(".Lif_{}_unhit:", my_cnt));
            gen_expr(x->rhs);
            push(format(".Lif_{}_end:", my_cnt));
        } else {
            push(format(".Lif_{}_unhit:", my_cnt));
        }
        return a0;
    }
    case N_WHILE: {
        int my_cnt = while_cnt++;

        push(format(".Lwhile_{}_begin:", my_cnt));
        a0 = gen_expr(x->cond);
        push(I_WHILE, my_cnt, a0);
        gen_expr(x->lhs);
        push(format("jmp .Lwhile_{}_begin", my_cnt));
        push(format(".Lwhile_{}_end:", my_cnt));
        
        return a0;
    }
    case N_FOR: {
        int my_cnt = for_cnt++;

        if (x->init)
            gen_expr(x->init);

        push(format(".Lfor_{}_begin:", my_cnt));
        a0 = gen_expr(x->cond);
        push(I_FOR, my_cnt, a0);
        gen_expr(x->lhs);
        if (x->step)
            gen_expr(x->step);
        push(format("jmp .Lfor_{}_begin", my_cnt));
        push(format(".Lfor_{}_end:", my_cnt));
        return a0;
    }
    default:
        throw x->ty;
    }
}

std::map<func*, std::vector<ir*>> generate() {
    decltype(generate()) p;
    for (auto f : funcs) {
        res.clear();
        if (f->body)
            gen_expr(f->body);
        p[f] = res;
    }
    
    return p;
}
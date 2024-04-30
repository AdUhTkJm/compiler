#include "ir.h"
#include "utils.h"
#include <map>
#define MAPPED std::make_pair
#define PUSH(x) res.push_back(new x)

std::map<node_type, ir_type> opmap {
    MAPPED(N_PLUS, I_ADD),
    MAPPED(N_MINUS, I_SUB),
    MAPPED(N_MUL, I_IMUL),
    MAPPED(N_DIV, I_IDIV),
    MAPPED(N_MOD, I_MOD),
};

ir::ir(ir_type ty, int imm, reg* a0):
    ty(ty), imm(imm), a0(a0), a1(nullptr), a2(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, reg* a2):
    ty(ty), a0(a0), a1(a1), a2(a2) {}

ir::ir(ir_type ty, reg* a0, var* v):
    ty(ty), a0(a0), v(v), a1(nullptr), a2(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, int sz):
    ty(ty), a0(a0), a1(a1), a2(nullptr), sz(sz) {}

int reg::cnt = 0;
reg::reg(): ind(cnt++) {}


// The result of generate()
std::vector<ir*> res;

reg* gen_imm(node* ast) {
    reg* a0 = new reg;
    PUSH(ir(I_IMM, ast->val, a0));
    return a0;
}

reg* gen_addr(var* v) {
    reg* a0 = new reg;

    if (v->is_global)
        PUSH(ir(I_GLOBALREF, a0, v));
    else
        PUSH(ir(I_LOCALREF, a0, v));

    return a0;
}

reg* gen_expr(node* ast) {
    // We cannot jump past initialisation of these variables in
    // switch(), so we declare them here.
    reg *a0, *a1;

    switch (ast->ty) {
    case N_NUM:
        return gen_imm(ast);
    case N_RET:
        a0 = gen_expr(ast->lhs);

        PUSH(ir(I_RET, a0));
        return a0;
    case N_PLUS:
    case N_MINUS:
    case N_MUL:
    case N_DIV:
    case N_MOD:
        a0 = gen_expr(ast->lhs);
        a1 = gen_expr(ast->rhs);
        
        PUSH(ir(opmap[ast->ty], a0, a1));
        return a0;
    case N_VARREF:
        a0 = new reg;
        a1 = gen_addr(ast->target);

        PUSH(ir(I_LOAD, a0, a1, ast->target->ty.sz));
        return a0;
    case N_ASSIGN:
        a0 = gen_addr(ast->target);
        a1 = gen_expr(ast->rhs);

        PUSH(ir(I_STORE, a0, a1, ast->target->ty.sz));
        return a0;
    case N_BLOCK:
        for (auto m : ast->nodes)
            gen_expr(m);
        return nullptr;
    case N_FCALL: {
        a0 = new reg;

        ir* i = new ir(I_CALL, a0);
        for (auto m : ast->nodes)
            i->params.push_back(gen_expr(m));
        i->name = ast->name;
        res.push_back(i);
        return a0;
    }
    default:
        throw ast->ty;
    }
}

std::map<func*, std::vector<ir*>> generate() {
    decltype(generate()) p;
    for (auto f : funcs) {
        res.clear();
        gen_expr(f->body);
        p[f] = res;
    }
    
    return p;
}
#include "ir.h"
#include "fmt/format.h"
#include <map>
#define MAPPED std::make_pair
#define PUSH(x) res.push_back(new x)

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

std::map<node_type, node_type> ndmap {
    MAPPED(N_PLUSEQ, N_PLUS),
    MAPPED(N_MINUSEQ, N_MINUS),
    MAPPED(N_MULEQ, N_MUL),
    MAPPED(N_DIVEQ, N_DIV),
    MAPPED(N_MODEQ, N_MOD),
    MAPPED(N_POSTINC, N_PLUSEQ),
    MAPPED(N_POSTDEC, N_MINUSEQ),
};

ir::ir(ir_type ty, int imm, reg* a0):
    ty(ty), imm(imm), a0(a0), a1(nullptr), a2(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, reg* a2):
    ty(ty), a0(a0), a1(a1), a2(a2) {}

ir::ir(ir_type ty, reg* a0, var* v):
    ty(ty), a0(a0), v(v), a1(nullptr), a2(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, int sz):
    ty(ty), a0(a0), a1(a1), a2(nullptr), sz(sz) {}

ir::ir(std::string str): ty(I_RAW), name(str) {}

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
    reg *a0, *a1, *a2;

    static int if_cnt = 0;
    static int while_cnt = 0;
    static int for_cnt = 0;

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
    case N_EQ:
    case N_LEQ:
    case N_GEQ:
    case N_NEQ:
    case N_GE:
    case N_LE:
        a0 = gen_expr(ast->lhs);
        a1 = gen_expr(ast->rhs);
        
        PUSH(ir(opmap[ast->ty], a0, a1));
        return a0;
    case N_PLUSEQ:
    case N_MINUSEQ:
    case N_MULEQ:
    case N_DIVEQ:
    case N_MODEQ:
        return gen_expr(new node(
            N_ASSIGN, ast->target,
            new node(ndmap[ast->ty], new node(N_VARREF, ast->target), ast->rhs)
        ));
    case N_POSTINC:
    case N_POSTDEC:
        a0 = gen_expr(new node(N_VARREF, ast->target));

        gen_expr(new node(ndmap[ast->ty], ast->target, new node(N_NUM, 1)));

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
        return a1; // !! We need value rather than address
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
    case N_IF:
        a0 = gen_expr(ast->cond);
        PUSH(ir(I_IF, if_cnt, a0));

        gen_expr(ast->lhs);

        if (ast->rhs) {
            PUSH(ir(format("jmp .Lif_{}_end", if_cnt)));
            PUSH(ir(format(".Lif_{}_unhit:", if_cnt)));
            gen_expr(ast->rhs);
            PUSH(ir(format(".Lif_{}_end:", if_cnt)));
        } else {
            PUSH(ir(format(".Lif_{}_unhit:", if_cnt)));
        }
        
        if_cnt++;
        return a0;
    case N_WHILE:
        PUSH(ir(format(".Lwhile_{}_begin:", while_cnt)));
        a0 = gen_expr(ast->cond);
        PUSH(ir(I_WHILE, while_cnt, a0));
        gen_expr(ast->lhs);
        PUSH(ir(format("jmp .Lwhile_{}_begin", while_cnt)));
        PUSH(ir(format(".Lwhile_{}_end:", while_cnt)));
        
        while_cnt++;
        return a0;
    case N_FOR:
        if (ast->init)
            gen_expr(ast->init);

        PUSH(ir(format(".Lfor_{}_begin:", for_cnt)));
        a0 = gen_expr(ast->cond);
        PUSH(ir(I_FOR, for_cnt, a0));
        gen_expr(ast->lhs);
        if (ast->step)
            gen_expr(ast->step);
        PUSH(ir(format("jmp .Lfor_{}_begin", for_cnt)));
        PUSH(ir(format(".Lfor_{}_end:", for_cnt)));

        for_cnt++;
        return a0;
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
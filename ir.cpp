#include "ir.h"
#include <map>
#define MAPPED std::make_pair
#define PUSH res.push_back

std::map<node_type, ir_type> opmap {
    MAPPED(N_PLUS, I_ADD),
    MAPPED(N_MINUS, I_SUB),
};

ir::ir(ir_type ty, int imm, reg* a0):
    ty(ty), imm(imm), a0(a0), a1(nullptr), a2(nullptr) {}

ir::ir(ir_type ty, reg* a0, reg* a1, reg* a2):
    ty(ty), a0(a0), a1(a1), a2(a2) {}

int reg::cnt = 0;
reg::reg(): ind(cnt++) {}


// The result of generate()
std::vector<ir> res;

reg* gen_imm(node* ast) {
    reg* a0 = new reg;
    PUSH(ir(I_IMM, ast->val, a0));
    return a0;
}

reg* gen_expr(node* ast) {
    switch (ast->ty) {
    case N_NUM:
        return gen_imm(ast);
    case N_PLUS:
    case N_MINUS:
        reg* a0 = gen_expr(ast->lhs);
        reg* a1 = gen_expr(ast->rhs);
        
        PUSH(ir(opmap[ast->ty], a0, a1));
        return a0;
    }

    return nullptr;
}

std::vector<ir> generate(node* ast) {
    gen_expr(ast);
    return res;
}
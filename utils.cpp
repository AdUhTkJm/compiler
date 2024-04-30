#include "utils.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <map>
using fmt::format;

#define MAPPED std::make_pair
std::map<node_type, const char*> showmap = {
    MAPPED(N_PLUS, "+"),
    MAPPED(N_MINUS, "-"),
    MAPPED(N_MUL, "*"),
    MAPPED(N_DIV, "/"),
    MAPPED(N_MOD, "%"),
    MAPPED(N_ASSIGN, "="),
};

void print_ast(std::ostream& os, node* ast, int depth) {
    os << std::flush;
    os << "; ";
    for (int i = 0; i < depth; i++)
        os << "  ";
    switch (ast->ty) {
    case N_NUM:
        os << format("num {}\n", ast->val);
        break;
    case N_VARREF:
        os << format("ref {}\n", ast->target->name);
        break;
    case N_PLUS:
    case N_MINUS:
    case N_MUL:
    case N_DIV:
    case N_MOD:
        os << showmap[ast->ty] << "\n";
        print_ast(os, ast->lhs, depth + 1);
        print_ast(os, ast->rhs, depth + 1);
        break;
    case N_ASSIGN:
        os << "=\n; ";
        for (int i = 0; i < depth + 1; i++)
            os << "  ";
        os << ast->target->name << "\n";
        print_ast(os, ast->rhs, depth + 1);
        break;
    case N_BLOCK:
        os << "block ->\n";
        for (auto m : ast->nodes)
            print_ast(os, m, depth + 1);
        break;
    case N_RET:
        os << "ret\n";
        print_ast(os, ast->lhs, depth + 1);
        break;
    case N_FCALL:
        os << format("{}()\n", ast->name);
        for (auto m : ast->nodes)
            print_ast(os, m, depth + 1);
        break;
    default:
        os << format("unknown value {}\n", (int) ast->ty);
    }
}

void print_ir(std::ostream& os, std::vector<ir*>& irs) {
    for (auto y : irs) {
        auto x = *y;
        os << "; ";
        switch (x.ty) {
        case I_IMM:
            os << format("IMM #{} = {}\n", x.a0->ind, x.imm);
            break;
        case I_ADD:
            os << format("ADD #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_SUB:
            os << format("SUB #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_IMUL:
            os << format("MUL #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_IDIV:
            os << format("DIV #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_MOD:
            os << format("MOD #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_LOCALREF:
            os << format("REF #{} = &{}\n", x.a0->ind, x.v->name);
            break;
        case I_GLOBALREF:
            os << format("REF #{} = &{} (G)\n", x.a0->ind, x.v->name);
            break;
        case I_STORE:
            os << format("MOV *(#{}) = #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_LOAD:
            os << format("MOV #{} = *(#{})\n", x.a0->ind, x.a1->ind);
            break;
        case I_RET:
            os << format("RET #{}\n", x.a0->ind);
            break;
        case I_CALL:
            os << format("CALL {}\n", x.name);
            break;
        default:
            os << format("UNKNOWN {}\n", (int) x.ty);
        }
    }
}
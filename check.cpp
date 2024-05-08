#include "check.h"
#include "fmt/format.h"
#include <algorithm>
#include <numeric>
#include <map>
#define MAPPED std::make_pair
using fmt::format;

std::map<std::string, std::vector<func*>> fs;

std::map<node_type, node_type> ndmap {
    MAPPED(N_PLUSEQ, N_PLUS),
    MAPPED(N_MINUSEQ, N_MINUS),
    MAPPED(N_MULEQ, N_MUL),
    MAPPED(N_DIVEQ, N_DIV),
    MAPPED(N_MODEQ, N_MOD),
    MAPPED(N_POSTINC, N_PLUSEQ),
    MAPPED(N_POSTDEC, N_MINUSEQ),
};

struct implicit {
    type* t;

    implicit(type* t): t(t) {}
};

bool operator==(type* ty, implicit im) {
    if (*ty == *im.t)
        return true;
    // int types can arbitrarily convert
    if (is_int_type(ty) && is_int_type(im.t))
        return true;
    // void* converts to every pointer type
    if (ty->ty == K_MUL && im.t->ty == K_MUL && (im.t->ptr_to->ty == K_VOID || ty->ptr_to->ty == K_VOID))
        return true;
    // T[] converts to T*
    if (im.t->ty == K_LBRACKET && ty->ty == K_MUL && *im.t->ptr_to == *ty->ptr_to)
        return true;

    return false;
}

// checks if the signature matches
bool signature(func* a, func* b) {
    const auto& ap = a->params;
    const auto& bp = b->params;
    if (ap.size() != bp.size())
        return false;
    
    for (int i = 0; i < ap.size(); i++)
        if (*ap[i]->ty != *bp[i]->ty)
            return false;
    
    return true;
}

template<typename... T>
void assert(bool x, std::string err, T... args) {
    if (!x)
        throw semantic_error(format(err, args...));
}

// Infers type of (a + b).
// There is no need to explicitly convert between int types.
type* infer_type(type* a, type* b) {
    // Currently only int types
    if (a->sz < b->sz)
        std::swap(a, b);
    assert(is_int_type(a) && is_int_type(b), "Incompatible types of binary operator");
    if (a->sz == 8)
        return new type(K_LONG);
    if (a->sz == 4)
        return new type(K_INT);
    if (a->sz == 2)
        return new type(K_SHORT);
    if (a->sz == 1)
        return new type(K_CHAR);
    throw semantic_error("Unknown type error");
}

void decay(node*& x) {
    if (x->cty->ty != K_LBRACKET)
        return;
    
    // Copy x; otherwise segmentation fault
    node* z = new node(*x);
    node y(N_ADDR, z);
    y.cty = type::ptr(x->cty->ptr_to);
    *x = y;
}

void check_node(func* f, node* x) {    
    switch (x->ty) {
    case N_NUM:
        x->cty = new type(K_INT);
        x->is_lval = false;
        break;
    case N_RET:
        if (!x->lhs) break;
        check_node(f, x->lhs);
        assert(f->ret == implicit(x->lhs->cty), "Return type error for {}", f->name);
        break;
    case N_DEREF:
        check_node(f, x->lhs);
        assert(x->lhs->cty->ty == K_MUL, "Dereferencing non-pointer");
        assert(x->lhs->cty->ptr_to->ty != K_VOID, "Cannot dereference void*");
        x->cty = x->lhs->cty->ptr_to;
        x->is_lval = true;
        decay(x); // x->lhs might itself be a pointer to array
        break;
    case N_ADDR:
        check_node(f, x->lhs);
        // When & is directly operating on an array, the array does not decay
        if (x->lhs->ty == N_ADDR && x->lhs->lhs->ty == N_VARREF && x->lhs->lhs->cty->ty == K_LBRACKET)
            x->lhs = x->lhs->lhs;
        
        assert(x->lhs->is_lval, "Lvalue expected");
        x->cty = type::ptr(x->lhs->cty);
        x->is_lval = false;
        break;
    case N_PLUS:
    case N_MINUS:
        check_node(f, x->lhs);
        check_node(f, x->rhs);
        // For pointer addition, we need to multiply another operand by the size of underlying type.
        // TODO: add special treatment for function pointers.
        if (x->lhs->cty->ty == K_MUL) {
            x->rhs = new node(N_MUL, x->rhs, new node(N_NUM, x->lhs->cty->ptr_to->sz));
            check_node(f, x->rhs);
            assert(is_int_type(x->rhs->cty), "Pointers addition is only compatible with int");
            x->cty = x->lhs->cty;
            break;
        } else if (x->rhs->cty->ty == K_MUL) {
            x->lhs = new node(N_MUL, x->lhs, new node(N_NUM, x->rhs->cty->ptr_to->sz));
            check_node(f, x->lhs);
            assert(is_int_type(x->lhs->cty), "Pointers addition is only compatible with int");
            x->cty = x->rhs->cty;
            break;
        } else {
            x->cty = infer_type(x->lhs->cty, x->rhs->cty);
        }
        x->is_lval = false;
        break;
    case N_MUL:
    case N_DIV:
    case N_MOD:
        check_node(f, x->lhs);
        check_node(f, x->rhs);
        x->cty = infer_type(x->lhs->cty, x->rhs->cty);
        x->is_lval = false;
        break;
    case N_EQ:
    case N_LEQ:
    case N_GEQ:
    case N_NEQ:
    case N_GE:
    case N_LE:
        check_node(f, x->lhs);
        check_node(f, x->rhs);
        // Either numerical or pointer
        assert(is_int_type(x->lhs->cty) && is_int_type(x->rhs->cty) ||
            *x->lhs->cty == *x->rhs->cty && x->lhs->cty->ty == K_MUL, "Incompatible types of equality test");
        x->cty = new type(K_INT);
        x->is_lval = false;
        break;
    case N_PLUSEQ:
    case N_MINUSEQ:
    case N_MULEQ:
    case N_DIVEQ:
    case N_MODEQ: {
        check_node(f, x->lhs);

        var* v = new var;
        v->ty = type::ptr(x->lhs->cty);
        f->v->push(v);

        node y(N_BLOCK);
        y.nodes = {
            new node(N_ASSIGN, new node(N_VARREF, v), new node(N_ADDR, x->lhs)),
            new node(N_ASSIGN, new node(N_DEREF, new node(N_VARREF, v)), new node(ndmap[x->ty], new node(N_DEREF, new node(N_VARREF, v)), x->rhs))
        };
        y.cty = x->lhs->cty;
        *x = y;
        check_node(f, x);
        break;
    }
    case N_POSTINC:
    case N_POSTDEC: {
        check_node(f, x->lhs);

        var* p = new var;
        p->ty = type::ptr(x->lhs->cty);
        f->v->push(p);

        var* v = new var;
        v->ty = x->lhs->cty;
        f->v->push(v);

        node y(N_BLOCK);
        y.nodes = {
            new node(N_ASSIGN, new node(N_VARREF, p), new node(N_ADDR, x->lhs)),
            new node(N_ASSIGN, new node(N_VARREF, v), new node(N_DEREF, new node(N_VARREF, p))),
            new node(ndmap[x->ty], new node(N_DEREF, new node(N_VARREF, p)), new node(N_NUM, 1)),
            new node(N_VARREF, v)
        };
        y.cty = x->lhs->cty;
        *x = y;
        check_node(f, x);
        break;
    }
    case N_VARREF:
        x->cty = x->target->ty;
        x->is_lval = true;
        decay(x);
        break;
    case N_ASSIGN:
        check_node(f, x->lhs);
        check_node(f, x->rhs);

        assert(x->lhs->is_lval, "Lvalue expected");
        assert(x->lhs->cty == implicit(x->rhs->cty), "Assignment type mismatch");
        assert(x->lhs->cty->ty != K_LBRACKET, "No assignment to array");
        assert(x->ignore_const || !x->lhs->cty->is_const, "No assignment to const variable");

        x->cty = x->lhs->cty;
        x->is_lval = false;
        break;
    case N_BLOCK:
        for (auto m : x->nodes)
            check_node(f, m);
        break;
    case N_FCALL: {
        assert(fs[x->name].size(), "Function {} not found", x->name);
            
        const auto& fn = fs[x->name][0];
        const auto& fp = fn->params;
        const auto& args = x->nodes;

        for (auto m : args)
            check_node(f, m);

        assert(fn->is_variadic || fp.size() == args.size(), "Argument count doesn't match for {}", fn->name);
        assert(fp.size() <= args.size(), "Function {} does not have enough arguments", fn->name);
        for (int i = 0; i < fp.size(); i++)
            assert(fp[i]->ty == implicit(args[i]->cty), "Arguments #{} don't match for {}", i, fn->name);
        x->val = args.size() - fp.size();
        x->is_lval = false;
        x->cty = fn->ret;
        break;
    }
    case N_IF:
        check_node(f, x->cond);
        assert(is_int_type(x->cond->cty), "Condition is not int");
        check_node(f, x->lhs);
        if (x->rhs)
            check_node(f, x->rhs);
        break;
    case N_WHILE:
        check_node(f, x->cond);
        assert(is_int_type(x->cond->cty), "Condition is not int");
        check_node(f, x->lhs);
        break;
    case N_FOR:
        check_node(f, x->cond);
        assert(is_int_type(x->cond->cty), "Condition is not int");
        if (x->init)
            check_node(f, x->init);
        if (x->step)
            check_node(f, x->step);
        check_node(f, x->lhs);
        break;
    default:
        assert(false, "Unknown node type {}", (int) x->ty);
    }
}

void check() {
    std::vector<func*> v;
    for (auto f : funcs)
        fs[f->name].push_back(f);
    
    for (auto [name, vf] : fs) {
        // Checks declaration
        func* rep = vf[0];
        for (auto f : vf)
            if (!signature(f, rep))
                throw new semantic_error(format("Overloading {}", name));
        
        int cnt = std::accumulate(vf.begin(), vf.end(), 0, [](int a, func* p) { return a + !!p->body; });
        if (cnt > 1)
            throw new semantic_error(format("Multiple definition of {}", name));
        
        if (cnt == 0)
            v.push_back(rep);
        else for (auto f : vf)
            if (f->body) {
                v.push_back(f);
                break;
            }
    }

    funcs = v;

    for (auto f : funcs)
        if (f->body)
            check_node(f, f->body);
}
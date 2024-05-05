#include "ast.h"
#include <algorithm>
#include <iostream>
#include <map>
#define MAPPED std::make_pair

std::vector<func*> funcs;

node::node(node_type ty, int val):
    ty(ty), val(val), is_lval(false) { }

node::node(node_type ty, node* lhs, node* rhs):
    ty(ty), lhs(lhs), rhs(rhs), is_lval(false) { }

node::node(node_type ty, var* target, node* rhs):
    ty(ty), target(target), rhs(rhs), is_lval(false) { }

env::env(env* father): father(father) { }

type::type(int ty): ty(ty), sz(get_size(ty)) { }

type::type(type* ptr): ty(K_MUL), sz(8), ptr_to(ptr) {  }

void env::push(var* v) {
    vars.push_back(v);
    if (father && father != global)
        father->push(v);
}

bool type::operator==(type x) {
    if (x.ty == K_MUL && ty == K_MUL)
        return *x.ptr_to == *ptr_to;
    
    return ty == x.ty;
}

bool type::operator!=(type x) {
    return !(*this == x);
}

int type::get_size(int x) {
    static std::map<int, int> szof {
        MAPPED(K_INT, 4),
        MAPPED(K_CHAR, 1),
        MAPPED(K_LONG, 8),
        MAPPED(K_SHORT, 2),
        MAPPED(K_VOID, 1),
    };
    return szof[x];
}

// The environment we are currently in.
env* envi = nullptr;
env* const global = new env;

// Determines if the next token is a type name.
// Does not consume.
bool is_type() {
    static token_type tys[] = {
        K_INT, K_CHAR, K_LONG, K_SHORT, K_VOID
    };
    token_type t = tin.peek().ty;

    if (std::find(tys, tys + sizeof tys, t) != tys + sizeof tys)
        return true;
    return false;
}

bool is_int_type(type* t) {
    static token_type tys[] = {
        K_INT, K_CHAR, K_LONG, K_SHORT
    };

    if (std::find(tys, tys + sizeof tys, t->ty) != tys + sizeof tys)
        return true;
    return false;
}

// Gets a simple type (i.e. not things like void (*)(int))
// Consumes all the way up to what's needed.
type* get_simple_type() {
    token t = tin.consume();
    type* res = new type(t.ty);
    switch (t.ty) {
    case K_INT:
        res->sz = 4;
        break;
    case K_LONG:
        res->sz = 8;
        break;
    case K_CHAR:
        res->sz = 1;
        break;
    case K_SHORT:
        res->sz = 2;
        break;
    case K_VOID:
        res->sz = 1; // So that void* behaves correctly.
        break;
    default:
        throw unexpected_token("Type not recognised");
    }

    while (test(K_MUL))
        res = new type(res);
    
    return res;
}

// Gets the declaration of the whole variable.
// Consumes all the way up to what's needed.
var* get_var(bool named = true) {
    var* res = new var;
    type* t = get_simple_type();
    
    if (t->ty == K_VOID)
        throw unexpected_token("Void is not a valid type for variables");

    res->ty = t;

    if (tin.peek().ty != K_IDENT)
        if (named)
            throw unexpected_token("Expected identifier");
        else return res;
    
    res->name = tin.consume().ident;
    return res;
}

var* resolve(std::string name) {
    for (env* b = envi; b; b = b->father)
        for (auto v : b->vars)
            if (v->name == name)
                return v;
    
    throw unexpected_token("Variable name resolution failed");
}

node* expr();
node* primary() {
    if (test(K_LBRACKET)) {
        node* t = expr();
        expect(K_RBRACKET);
        return t;
    }

    if (tin.peek().ty == K_IDENT) {
        token t = tin.consume();

        // Function call
        if (test(K_LBRACKET)) {
            node* k = new node(N_FCALL);

            k->name = t.ident;

            if (!test(K_RBRACKET)) {
                do k->nodes.push_back(expr()); while (test(K_COMMA));
                expect(K_RBRACKET);
            }

            return k;
        }

        // Variable reference
        return new node(N_VARREF, resolve(t.ident));
    }

    token x = tin.consume();
    if (x.ty == K_NUM)
        return new node(N_NUM, x.val);

    throw unexpected_token("Unexpected primary()");
}

node* unary() {
    token k = tin.peek();
    if (test(K_PP) || test(K_MM)) {
        node* t = unary();
        if (t->ty != N_VARREF)
            throw unexpected_token("Expected identifier after ++/--");
        else
            return new node(k.ty == K_PP ? N_PLUSEQ : N_MINUSEQ, t, new node(N_NUM, 1));
    }

    if (test(K_MINUS))
        return new node(N_MINUS, new node(N_NUM, 0), unary());
    if (test(K_PLUS))
        ;

    if (test(K_MUL))
        return new node(N_DEREF, unary());
    if (test(K_AND))
        return new node(N_ADDR, unary());

    node* t = primary();
    
    k = tin.peek();
    if (test(K_PP) || test(K_MM))
        if (t->ty != N_VARREF)
            throw unexpected_token("Expected identifier before ++/--");
        else
            return new node(k.ty == K_PP ? N_POSTINC : N_POSTDEC, t);
    
    return t;
}

node* factor() {
    node* t = unary();

    if (test(K_MUL))
        return new node(N_MUL, t, factor());
    if (test(K_DIV))
        return new node(N_DIV, t, factor());
    if (test(K_MOD))
        return new node(N_MOD, t, factor());
    
    return t;
}

node* term() {
    node* t = factor();
    if (test(K_MINUS))
        return new node(N_MINUS, t, term());
    if (test(K_PLUS))
        return new node(N_PLUS, t, term());
    
    return t;
}

node* less() {
    node* t = term();
    if (test(K_LEQ))
        return new node(N_LEQ, t, term());
    if (test(K_GEQ))
        return new node(N_GEQ, t, term());
    if (test(K_LE))
        return new node(N_LE, t, term());
    if (test(K_GE))
        return new node(N_GE, t, term());
    
    return t;
}

node* eq() {
    node* t = less();
    if (test(K_EQ))
        return new node(N_EQ, t, term());
    if (test(K_NEQ))
        return new node(N_NEQ, t, term());

    return t;
}

node* assign() {
    node* t = eq();
    if (test(K_ASSIGN))
        return new node(N_ASSIGN, t, expr());
    if (test(K_PLUSEQ))
        return new node(N_PLUSEQ, t, expr());
    if (test(K_MINUSEQ))
        return new node(N_MINUSEQ, t, expr());
    if (test(K_MULEQ))
        return new node(N_MULEQ, t, expr());
    if (test(K_DIVEQ))
        return new node(N_DIVEQ, t, expr());
    if (test(K_MODEQ))
        return new node(N_MODEQ, t, expr());
    
    return t;
}

node* expr() {
    return assign();
}

node* stmt() {
    // 'return' statement
    if (test(K_RET)) {
        // empty return
        if (test(K_SEMICOLON))
            return new node(N_RET);

        node* t = expr();
        expect(K_SEMICOLON);
        return new node(N_RET, t);
    }

    // Declaration
    if (is_type()) {
        var* v = get_var();
        envi->push(v);

        node* r = nullptr;
        if (test(K_ASSIGN))
            r = new node(N_ASSIGN, new node(N_VARREF, v), expr());
        
        expect(K_SEMICOLON);
        return r;
    }

    // if-statement
    if (test(K_IF)) {
        node* t = new node(N_IF);
        expect(K_LBRACKET);
        t->cond = expr();
        expect(K_RBRACKET);
        t->lhs = stmt();

        if (test(K_ELSE))
            t->rhs = stmt();
        
        return t;
    }

    // while-statement
    if (test(K_WHILE)) {
        node* t = new node(N_WHILE);
        expect(K_LBRACKET);
        t->cond = expr();
        expect(K_RBRACKET);
        t->lhs = stmt();
        return t;
    }

    // for-statement
    if (test(K_FOR)) {
        node* t = new node(N_FOR);
        expect(K_LBRACKET);

        env* old = envi;
        env* v = new env(envi);
        envi = v;

        // init
        if (!test(K_SEMICOLON)) {
            // (1) Variable declaration
            if (is_type())
                t->init = stmt();
            
            // (2) Any other expression
            else {
                t->init = expr();
                expect(K_SEMICOLON);
            }
        } else t->init = nullptr;

        // cond
        if (!test(K_SEMICOLON)) {
            t->cond = expr();
            expect(K_SEMICOLON);
        } else t->cond = new node(N_NUM, 1);
        
        // step
        if (!test(K_RBRACKET)) {
            t->step = expr();
            expect(K_RBRACKET);
        } else t->step = nullptr;

        t->lhs = stmt();
        envi = old;
        return t;
    }
    
    // Block
    if (test(K_LBRACE)) {
        env* old = envi;
        env* _new = new env(old);
        envi = _new;

        node* t = new node(N_BLOCK);

        while (!test(K_RBRACE))
            if (auto k = stmt(); k)
                t->nodes.push_back(k);
        
        envi = old;
        return t;
    }

    node* t = expr();
    expect(K_SEMICOLON);
    return t;
}

void parse() {
    while (!test(K_EOF)) {
        if (!is_type())
            throw unexpected_token("Typename expected");

        bool isfunc = false;
        
        tin.save();
        get_simple_type();
        expect(K_IDENT);
        isfunc = test(K_LBRACKET);
        tin.load();

        // declarations not yet supported.
        if (isfunc) {
            func* f = new func;
            f->ret = get_simple_type();
            f->name = tin.consume().ident;
            f->v = envi = new env(global);
            expect(K_LBRACKET);
            if (!test(K_RBRACKET)) {
                do {
                    var* v = get_var(false);
                    v->is_param = true;
                    f->params.push_back(v);
                    f->v->push(v);
                } while (test(K_COMMA));
                expect(K_RBRACKET);
            }
            
            if (test(K_SEMICOLON)) {
                f->body = nullptr;
                envi = global;

                funcs.push_back(f);
                continue;
            }
            if (tin.peek().ty != K_LBRACE)
                throw unexpected_token("Expected {");
                
            f->body = stmt();
            envi = global;

            funcs.push_back(f);
            continue;
        }
        
        var* v = get_var();
        v->is_global = true;
        global->vars.push_back(v);
        expect(K_SEMICOLON);
    }
}
#include "ast.h"
#include <algorithm>

std::vector<func*> funcs;

node::node(node_type ty, int val):
    ty(ty), val(val) { }

node::node(node_type ty, node* lhs, node* rhs):
    ty(ty), lhs(lhs), rhs(rhs) { }

node::node(node_type ty, var* target, node* rhs):
    ty(ty), target(target), rhs(rhs) { }

env::env(env* father): father(father) { }

void env::push(var* v) {
    vars.push_back(v);
    if (father && father != global)
        father->push(v);
}

// The environment we are currently in.
env* envi = nullptr;
env* const global = new env;

// Determines if the next token is a type name.
// Does not consume.
bool is_type() {
    static token_type tys[] = {
        K_INT
    };
    token_type t = tin.peek().ty;

    if (std::find(tys, tys + sizeof tys, t) != tys + sizeof tys)
        return true;
    return false;
}

// Gets a simple type (i.e. not things like void (*)(int))
// Consumes all the way up to what's needed.
type get_simple_type() {
    token t = tin.consume();
    type res;
    switch (t.ty) {
    case K_INT:
        res.ty = T_INT;
        res.sz = 4;
        break;
    default:
        throw unexpected_token("Type not recognised");
    }
    return res;
}

// Gets the declaration of the whole variable.
// Consumes all the way up to what's needed.
var* get_var() {
    var* res = new var;
    type t = get_simple_type();
    // Ready to test for (*) etc.

    // Simple declaration
    res->ty = t;
    if (tin.peek().ty != K_IDENT)
        throw unexpected_token("Expected identifier");
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
        return new node(N_VARREF, resolve(t.ident));
    }

    token x = tin.consume();
    if (x.ty == K_NUM)
        return new node(N_NUM, x.val);

    throw unexpected_token("Unexpected primary()");
}

node* factor() {
    node* t = primary();
    token op = tin.consume();

    if (op.ty == K_MUL)
        return new node(N_MUL, t, factor());
    if (op.ty == K_DIV)
        return new node(N_DIV, t, factor());
    if (op.ty == K_MOD)
        return new node(N_MOD, t, factor());
    
    tin.retreat();
    return t;
}

node* term() {
    node* t = factor();
    token op = tin.consume();
    if (op.ty == K_MINUS)
        return new node(N_MINUS, t, term());
    if (op.ty == K_PLUS)
        return new node(N_PLUS, t, term());
    
    tin.retreat();
    return t;
}

node* assign() {
    node* t = term();
    token op = tin.consume();
    if (op.ty == K_ASSIGN)
        return new node(N_ASSIGN, t->target, expr());

    tin.retreat();
    return t;
}

node* expr() {
    return assign();
}

node* stmt() {
    // 'return' statement
    if (test(K_RET)) {
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
            r = new node(N_ASSIGN, v, expr());
        
        expect(K_SEMICOLON);
        return r;
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
            expect(K_LBRACKET);
            if (!test(K_RBRACKET)) {
                do f->params.push_back(get_var()); while (test(K_COMMA));
                expect(K_RBRACKET);
            }
            
            f->v = new env;
            envi = f->v;
            f->body = stmt();
            envi = global;

            funcs.push_back(f);
            return;
        }
        
        var* v = get_var();
        v->is_global = true;
        global->vars.push_back(v);
        expect(K_SEMICOLON);
    }
}
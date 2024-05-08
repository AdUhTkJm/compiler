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

type::type(int ty): ty(ty), sz(get_size(ty)), is_const(false) { }

type* type::ptr(type* p) {
    type* t = new type(K_MUL);
    t->ptr_to = p;
    t->sz = 8;
    t->is_const = false;
    return t;
}

type* type::arr(type* p, int sz) {
    type* t = new type(K_LBRACKET);
    t->ptr_to = p;
    t->asz = sz;
    t->sz = sz * p->sz;
    t->is_const = true;
    return t;
}

type* type::fn(type* ret, std::vector<type*> param) {
    type* t = new type(K_LPARENS);
    t->params = param;
    t->ret = ret;
    t->sz = 8;
    t->is_const = true;
    return t;
}

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
        MAPPED(K_MUL, 8),
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

// Determines if the next token is a type name or a cv_qualifier.
// Does not consume.
bool is_cv_type() {
    static token_type cv[] = {
        K_CONST
    };
    token_type t = tin.peek().ty;

    if (std::find(cv, cv + sizeof cv, t) != cv + sizeof cv)
        return true;
    return is_type();
}

bool is_int_type(type* t) {
    static token_type tys[] = {
        K_INT, K_CHAR, K_LONG, K_SHORT
    };

    if (std::find(tys, tys + sizeof tys, t->ty) != tys + sizeof tys)
        return true;
    return false;
}

void qualify(type* ty) {
    while (test(K_CONST))
        ty->is_const = true;
}

type* type_spec() {
    type* res = new type;

    qualify(res);
    if (!is_type())
        throw unexpected_token("Type not recognised");
    token t = tin.consume();
    res->ty = t.ty;
    res->sz = type::get_size(t.ty);
    qualify(res);

    return res;
}

// For function return types.
// They only return simple types or a pointer.
type* read_ptr() {
    type* ty = type_spec();
    while (test(K_MUL))
        ty = type::ptr(ty);
    return ty;
}

type* decl(type*, std::string&);
type* full_decl(std::string&);
type* direct_decl(type* base, std::string& name) {
    bool delayed = false;
    int ret_point = 0;
    if (test(K_LPARENS)) {
        ret_point = tin.save();
        while (!test(K_RPARENS))
            tin.consume();
        delayed = true;
    }
    type* ty = base;
    qualify(ty);

    token t = tin.peek();
    if (t.ty == K_IDENT) {
        name = t.ident;
        tin.consume();
    }
    
    bool is_array = false;
    while (test(K_LBRACKET)) {
        is_array = true;
        if (test(K_RBRACKET)) {
            ty = type::arr(ty, -1);
            continue;
        }
        
        token t = tin.consume();
        if (t.ty != K_NUM)
            throw unexpected_token("Array length must be constant");
        if (t.val <= 0)
            throw unexpected_token("Expected positive array length");
        ty = type::arr(ty, t.val);
        expect(K_RBRACKET);
    }
    
    if (is_array && test(K_LPARENS))
        throw unexpected_token("Array of functions is not allowed");
    
    if (test(K_LPARENS) && !test(K_RPARENS)) {
        std::vector<type*> param;
        do {
            std::string no_name;
            type* t = full_decl(no_name);
            if (no_name != "")
                throw unexpected_token("Arguments can't be named for function pointers");
            param.push_back(t);
        } while (test(K_COMMA));

        expect(K_RPARENS);

        ty = type::fn(ty, param);
    }

    if (delayed) {
        int end_point = tin.save();
        tin.load(ret_point);
        ty = decl(ty, name);
        expect(K_RPARENS);
        tin.load(end_point);
    }

    return ty;
}

type* decl(type* base, std::string& name) {
    type* ty = base;
    while (test(K_MUL))
        ty = type::ptr(ty);

    return direct_decl(ty, name);
}

type* full_decl(std::string& name) {
    type* base = type_spec();
    return decl(base, name);
}

var* get_var(bool must_name = true) {
    var* v = new var;
    v->ty = full_decl(v->name);
    if (must_name && v->name == "")
        throw unexpected_token("Expected variable name");
    return v;
}

// If base is null, then fulfuill base;
// If not, then take base as granted and do not call type_spec()
var* get_base_var(type*& base) {
    var* v = new var;

    if (!base)
        base = type_spec();
    v->ty = decl(base, v->name);
    if (v->name == "")
        throw unexpected_token("Expected variable name");
    return v;
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
    static int str_cnt = 0;

    if (test(K_LPARENS)) {
        node* t = expr();
        expect(K_RPARENS);
        return t;
    }

    if (tin.peek().ty == K_IDENT) {
        token t = tin.consume();

        // Function call
        if (test(K_LPARENS)) {
            node* k = new node(N_FCALL);

            k->name = t.ident;

            if (!test(K_RPARENS)) {
                do k->nodes.push_back(expr()); while (test(K_COMMA));
                expect(K_RPARENS);
            }

            return k;
        }

        // Variable reference
        return new node(N_VARREF, resolve(t.ident));
    }

    token x = tin.consume();

    // String literal
    if (x.ty == K_STR) {
        var* v = new var;
        v->name = "__builtin_str_" + std::to_string(++str_cnt);
        v->ty = new type(K_CHAR);
        v->is_strlit = true;
        v->is_global = true;
        v->value = x.ident;
        global->push(v);
        return new node(N_ADDR, new node(N_VARREF, v));
    }

    // Numeric literal
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
        return unary(); // Can't ignore here; otherwise (+- -+2) will not work

    if (test(K_MUL))
        return new node(N_DEREF, unary());
    if (test(K_AND))
        return new node(N_ADDR, unary());

    node* t = primary();
    
    k = tin.peek();
    // Note: a++++ isn't allowed
    if (test(K_PP) || test(K_MM))
        if (t->ty != N_VARREF)
            throw unexpected_token("Expected identifier before ++/--");
        else
            return new node(k.ty == K_PP ? N_POSTINC : N_POSTDEC, t);
    // Left grouping, same as +-*/%
    while (test(K_LBRACKET)) {
        node* rhs = expr();
        expect(K_RBRACKET);
        t = new node(N_DEREF, new node(N_PLUS, t, rhs));
    }

    return t;
}

node* factor() {
    node* t = unary();

    for (token_type ty = tin.peek().ty; ty == K_MUL || ty == K_DIV || ty == K_MOD; ty = tin.peek().ty) {
        tin.consume();
        t = new node(ty == K_MUL ? N_MUL : ty == K_DIV ? N_DIV : N_MOD, t, unary());
    }
    
    return t;
}

node* term() {
    node* t = factor();

    for (token_type ty = tin.peek().ty; ty == K_PLUS || ty == K_MINUS; ty = tin.peek().ty) {
        tin.consume();
        t = new node(ty == K_PLUS ? N_PLUS : N_MINUS, t, factor());
    }
    
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
    if (is_cv_type()) {
        type* base = nullptr;
        // Note that as long as envi does not change,
        // this does not influence scope
        node* r = new node(N_BLOCK);
        var* v;
        
        do {
            v = get_base_var(base);
            envi->push(v);

            if (test(K_ASSIGN)) {
                node* t = new node(N_ASSIGN, new node(N_VARREF, v), expr());
                t->ignore_const = true;
                r->nodes.push_back(t);
            }

        } while (test(K_COMMA));
        
        expect(K_SEMICOLON);
        return r;
    }

    // if-statement
    if (test(K_IF)) {
        node* t = new node(N_IF);
        expect(K_LPARENS);
        t->cond = expr();
        expect(K_RPARENS);
        t->lhs = stmt();

        if (test(K_ELSE))
            t->rhs = stmt();
        
        return t;
    }

    // while-statement
    if (test(K_WHILE)) {
        node* t = new node(N_WHILE);
        expect(K_LPARENS);
        t->cond = expr();
        expect(K_RPARENS);
        t->lhs = stmt();
        return t;
    }

    // for-statement
    if (test(K_FOR)) {
        node* t = new node(N_FOR);
        expect(K_LPARENS);

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
        if (!test(K_RPARENS)) {
            t->step = expr();
            expect(K_RPARENS);
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
        read_ptr();
        while (test(K_MUL));
        isfunc = test(K_IDENT) && test(K_LPARENS);
        tin.load();

        if (isfunc) {
            func* f = new func;
            f->ret = read_ptr();
            f->name = tin.consume().ident;
            f->v = envi = new env(global);
            expect(K_LPARENS);
            if (!test(K_RPARENS)) {
                do {
                    // Variadic arguments; must be final one in argument list
                    if (test(K_DOTS)) {
                        f->is_variadic = true;
                        break;
                    }
                    var* v = get_var(false);
                    v->is_param = true;
                    f->params.push_back(v);
                    f->v->push(v);
                } while (test(K_COMMA));
                expect(K_RPARENS);
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
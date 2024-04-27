#include "ast.h"

node::node(node_type ty, int val):
    ty(ty), val(val) { }

node::node(node_type ty, node* lhs, node* rhs):
    ty(ty), lhs(lhs), rhs(rhs) { }

node* expr();
node* primary() {
    if (test(K_LBRACKET)) {
        node* t = expr();
        expect(K_RBRACKET);
        return t;
    }

    token x = tin.consume();
    if (x.ty == K_NUM)
        return new node(N_NUM, x.val);

    throw unexpected_token();
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

node* expr() {
    return term();
}

node* stmt() {
    // 'return' statement
    if (test(K_RET)) {
        node* t = term();
        expect(K_SEMICOLON);
        return new node(N_RET, t);
    }

    node* t = term();
    expect(K_SEMICOLON);
    return t;
}

node* parse() {
    return stmt();
}
#include "ast.h"

node::node(node_type ty, int val):
    ty(ty), val(val) { }

node::node(node_type ty, node* lhs, node* rhs):
    ty(ty), lhs(lhs), rhs(rhs) { }

node* primary() {
    token x = tin.consume();

    if (x.ty == K_NUM)
        return new node(N_NUM, x.val);

    throw unexpected_token();
}

node* term() {
    node* t = primary();
    token op = tin.consume();
    if (op.ty == K_MINUS)
        return new node(N_MINUS, t, term());
    if (op.ty == K_PLUS)
        return new node(N_PLUS, t, term());
    
    tin.retreat();
    return t;
}

node* parse() {
    return term();
}
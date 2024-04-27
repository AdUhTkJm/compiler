#pragma once
#include "lexer.h"

enum node_type {
    N_NUM,          // number, eg 0
    N_PLUS,         // +
    N_MINUS,        // -
    N_MUL,          // *
    N_DIV,          // /
    N_MOD,          // %
    N_RET,          // return
};

// A node of AST.
struct node {
    node_type ty;
    int val;

    node* lhs;
    node* rhs;

    node(node_type ty, int val);
    node(node_type ty, node* lhs, node* rhs=nullptr);
};

// Parse the AST from tokens stored in tin.
node* parse();
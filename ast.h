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
    N_ASSIGN,       // =
    N_VARREF,       // a;
    N_BLOCK,        // { ... }
};

enum type_type {
    T_INT,
};

struct type {
    type_type ty;
    int sz;
};

struct var {
    type ty;
    std::string name;
    
    bool is_global;

    int offset;

    var(): is_global(false) {}
};

// Environment
struct env {
    env* father;
    std::vector<var*> vars;

    void push(var*);

    env(env* father=nullptr);
};

// A node of AST.
struct node {
    node_type ty;
    int val;

    node* lhs;
    node* rhs;

    var* target;

    std::vector<node*> nodes;

    node(node_type ty, int val);
    node(node_type ty, node* lhs=nullptr, node* rhs=nullptr);
    node(node_type ty, var* target, node* rhs=nullptr);
};

struct func {
    std::string name;
    
    node* body;
    env* v;
    std::vector<var*> params;
    type ret;
};

extern std::vector<func*> funcs;
extern env* const global;

// Parse the AST from tokens stored in tin.
// Data is stored in funcs and globals.
void parse();
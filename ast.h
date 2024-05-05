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
    N_FCALL,        // main();
    N_IF,           // if
    N_FOR,          // for
    N_WHILE,        // while
    N_LE,           // <
    N_GE,           // >
    N_LEQ,          // <=
    N_GEQ,          // >=
    N_NEQ,          // !=
    N_EQ,           // ==
    N_PLUSEQ,       // +=
    N_MINUSEQ,      // -=
    N_MULEQ,        // *=
    N_DIVEQ,        // /=
    N_MODEQ,        // %=
    N_POSTINC,      // post ++
    N_POSTDEC,      // post --
    N_DEREF,        // *p
    N_ADDR,         // &a
};

struct type {
    int ty;
    int sz;

    type* ptr_to;

    explicit type(int=0);
    explicit type(type*);

    bool operator==(type);
    bool operator!=(type);

    static int get_size(int);
};

struct var {
    type* ty;
    std::string name;
    
    bool is_global;
    bool is_param;

    int offset;

    var(): is_global(false), is_param(false), offset(0) {}
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
    
    // Value of number literals
    int val;

    // Operands
    node* lhs;
    node* rhs;

    // Target of N_VARREF
    var* target;

    // For N_BLOCK, this is all statements;
    // For N_FCALL, this is all parameters
    std::vector<node*> nodes;
    
    // Name of the function to call
    std::string name;

    // condition of if/while/for
    node* cond;

    // init and step expression of for
    // for if/while/for, their block is in lhs
    // the else block is in rhs
    node* init;
    node* step;

    // for semantics check
    bool is_lval;
    // means "type in C"
    type* cty;
    

    node(node_type ty, int val);
    node(node_type ty, node* lhs=nullptr, node* rhs=nullptr);
    node(node_type ty, var* target, node* rhs=nullptr);
};

struct func {
    std::string name;
    
    node* body;
    env* v;
    type* ret;

    std::vector<var*> params;
};

extern std::vector<func*> funcs;
extern env* const global;

// Parse the AST from tokens stored in tin.
// Data is stored in funcs and globals.
void parse();

bool is_int_type(type*);
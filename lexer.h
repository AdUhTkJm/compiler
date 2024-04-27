#pragma once
#include <string>
#include <vector>

enum token_type {
    K_NUM,          // number literal
    K_PLUS,         // +
    K_MINUS,        // -
    K_MUL,          // *
    K_DIV,          // /
    K_MOD,          // %
    K_SEMICOLON,    // ;
    K_RET,          // return
    K_LBRACKET,     // (
    K_RBRACKET,     // )
};

struct token {
    token_type ty;
    int val;
};

struct unexpected_token {};

class tstream {
    std::vector<token> tokens;
    int curr;
public:
    void tokenize(const std::string&);
    void retreat() { curr--; }
    token peek() { return tokens[curr]; }
    token consume() { return tokens[curr++]; }

    // For debug uses.
    void _print();
};

extern tstream tin;

// Consumes the next token and sees if it is t. Throws if not.
void expect(token_type t);

// Peeks the next token and sees if it is t.
// If it is, then consume it.
bool test(token_type t);
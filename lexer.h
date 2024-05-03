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
    K_INT,          // int
    K_ASSIGN,       // =
    K_IDENT,        // identifier
    K_LBRACE,       // {
    K_RBRACE,       // }
    K_COMMA,        // ,
    K_EOF,          // end of file
    K_IF,           // if
    K_ELSE,         // else
    K_FOR,          // for
    K_WHILE,        // while
    K_LE,           // <
    K_GE,           // >
    K_LEQ,          // <=
    K_GEQ,          // >=
    K_EQ,           // ==
    K_NEQ,          // !=
    K_PLUSEQ,       // +=
    K_MINUSEQ,      // -=
    K_MULEQ,        // *=
    K_DIVEQ,        // /=
    K_MODEQ,        // %=
    K_PP,           // ++
    K_MM,           // --
    K_LONG,         // long
    K_SHORT,        // short
    K_CHAR,         // char
    K_VOID,         // void
};

struct token {
    token_type ty;
    int val;

    std::string ident;
};

struct unexpected_token: std::exception {
    std::string desc;
    
    const char* what() const noexcept override {
        return desc.c_str();
    }

    unexpected_token(): desc("") {}
    unexpected_token(std::string desc): desc(desc) {}
};

class tstream {
    std::vector<token> tokens;
    int curr;
    int memory;
public:
    void tokenize(const std::string&);
    void retreat() { curr--; }
    token peek() { return tokens[curr]; }
    token consume() { return tokens[curr++]; }

    void save() { memory = curr; }
    void load() { curr = memory; }

    void seteof() { tokens.push_back({ K_EOF }); }

    // For debug uses.
    void _print();
};

extern tstream tin;

// Consumes the next token and sees if it is t. Throws if not.
void expect(token_type t);

// Peeks the next token and sees if it is t.
// If it is, then consume it.
bool test(token_type t);
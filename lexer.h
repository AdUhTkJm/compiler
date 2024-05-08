#pragma once
#include <string>
#include <vector>

enum token_type {
    K_NUM,          // number literal
    K_PLUS,         // +
    K_MINUS,        // -
    K_MUL,          // *; also used as type of pointers
    K_DIV,          // /
    K_MOD,          // %
    K_SEMICOLON,    // ;
    K_RET,          // return
    K_LPARENS,      // (
    K_RPARENS,      // )
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
    K_AND,          // &
    K_LBRACKET,     // [; also used as type of arrays
    K_RBRACKET,     // ]
    K_CONST,        // const
    K_STR,          // "a string"
    K_DOTS,         // ...
};

struct token {
    token_type ty;
    int val;

    // For K_IDENT, this is identifier
    // For K_STR, this is the content of the string literal
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
    void tokenize(std::string);
    void retreat() { curr--; }
    token peek() { return tokens[curr]; }
    token consume() { return tokens[curr++]; }

    int save() { return memory = curr; }
    void load(int x) { curr = x; }
    void load() { curr = memory; }

    void seteof() { tokens.push_back({ K_EOF }); }
};

extern tstream tin;

// Consumes the next token and sees if it is t. Throws if not.
void expect(token_type t);

// Peeks the next token and sees if it is t.
// If it is, then consume it.
bool test(token_type t);
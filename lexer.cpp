#include "lexer.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <map>
#define MAPPED std::make_pair

tstream tin;

std::map<std::string, token_type> tkmap {
    MAPPED("return", K_RET),
    MAPPED("if", K_IF),
    MAPPED("else", K_ELSE),
    MAPPED("for", K_FOR),
    MAPPED("while", K_WHILE),
    MAPPED("int", K_INT),
    MAPPED("long", K_LONG),
    MAPPED("short", K_SHORT),
    MAPPED("char", K_CHAR),
    MAPPED("void", K_VOID),
    MAPPED("+", K_PLUS),
    MAPPED("-", K_MINUS),
    MAPPED("*", K_MUL),
    MAPPED("/", K_DIV),
    MAPPED("%", K_MOD),
    MAPPED(";", K_SEMICOLON),
    MAPPED("(", K_LBRACKET),
    MAPPED(")", K_RBRACKET),
    MAPPED("{", K_LBRACE),
    MAPPED("}", K_RBRACE),
    MAPPED(",", K_COMMA),
    MAPPED("=", K_ASSIGN),
    MAPPED("<", K_LE),
    MAPPED("<=", K_LEQ),
    MAPPED(">", K_GE),
    MAPPED(">=", K_GEQ),
    MAPPED("==", K_EQ),
    MAPPED("!=", K_NEQ),
    MAPPED("+=", K_PLUSEQ),
    MAPPED("-=", K_MINUSEQ),
    MAPPED("*=", K_MULEQ),
    MAPPED("/=", K_DIVEQ),
    MAPPED("%=", K_MODEQ),
    MAPPED("++", K_PP),
    MAPPED("--", K_MM),
    MAPPED("&", K_AND),
};

void tstream::tokenize(const std::string& what) {
    static std::string ext[] = {
        "+", "-", "<", ">", "=", "*", "/", "%", "!"
    };
    for (int i = 0; i < what.size();) {
        char x = what[i];

        if (isblank(x)) {
            i++;
            continue;
        }

        // Reads an identifier - or a keyword.
        if (isalpha(x)) {
            std::string str;
            while (isalpha(what[i]))
                str.push_back(what[i++]);
            
            if (tkmap.find(str) != tkmap.end())
                tokens.push_back({ tkmap[str] });
            else {
                token t { K_IDENT };
                t.ident = str;
                tokens.push_back(t);
            }
            continue;
        }

        // Reads a number.
        if (isdigit(x)) {
            int val = 0;
            while (isdigit(what[i]))
                val = 10 * val + what[i++] - '0';
            tokens.push_back({ K_NUM, val });
            continue;
        }

        // Reads an operator.
        std::string s = std::string() + x;
        if (what[i + 1] == '=' && std::find(ext, ext + sizeof(ext), s) != ext + sizeof(ext))
            s += what[++i];
        if (what[i] == '+' && what[i + 1] == '+' || what[i] == '-' && what[i + 1] == '-')
            s += what[++i];

        // tokenize() only deals with a single line.
        if (what[i] == '/' && what[i + 1] == '/')
            return;
            
        tokens.push_back({ tkmap[s] });
        i++;
    }
}

void tstream::_print() {
    for (auto t : tokens)
        std::cout << fmt::format("{}: {}\n", (int) t.ty, t.val);
}

void expect(token_type t) {
    if (tin.consume().ty != t)
        throw unexpected_token(std::string("Expected ") + std::to_string((int) t));
}

bool test(token_type t) {
    if (tin.peek().ty == t) {
        tin.consume();
        return true;
    }
    return false;
}
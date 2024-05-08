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
    MAPPED("(", K_LPARENS),
    MAPPED(")", K_RPARENS),
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
    MAPPED("[", K_LBRACKET),
    MAPPED("]", K_RBRACKET),
    MAPPED("const", K_CONST),
};

std::map<char, char> escape {
    MAPPED('n', '\n'),
    MAPPED('a', '\a'),
    MAPPED('t', '\t'),
    MAPPED('0', '\0'),
    MAPPED('b', '\b'),
    MAPPED('r', '\r'),
    MAPPED('f', '\f'),
    MAPPED('v', '\v'),
    MAPPED('\\', '\\'),
    MAPPED('\'', '\''),
    MAPPED('\"', '\"'),
};

bool is_in_comment = false;

// hexadecimal
int hex(char x) {
    if (x >= 'A' && x <= 'F')
        return (x - 'A' + 10);
    if (isdigit(x))
        return x - '0';
    
    throw unexpected_token("Bad hexadecimal");
}

// octal
int oct(char x) {
    if (x >= '0' && x <= '7')
        return x - '0';

    throw unexpected_token("Bad octal");
}

// binary
int bin(char x) {
    if (x == '0')
        return 0;
    if (x == '1')
        return 1;

    throw unexpected_token("Bad binary");
}

// Process an escape sequence and return a char.
// Moves i to the next character.
char escaped(const std::string& what, int& i) {
    if (what[i] != '\\')
        return what[i++];
    
    ++i;
    // hexadecmial
    if (what[i] == 'x') {
        if (i + 2 >= what.size() - 1)
            throw unexpected_token("Escape sequence not finished");
        int x = hex(what[i + 1]) * 16 + hex(what[i + 2]);
        
        i += 3;
        return x;
    }
    // octal
    if (isdigit(what[i])) {
        if (i + 2 >= what.size() - 1)
            throw unexpected_token("Escape sequence not finished");
        // Pay special attention to \0
        if (what[i] == '0' && !(what[i + 1] >= '0' && what[i + 1] <= '7')) {
            i++;
            return '\0';
        }
        int x = oct(what[i]) * 64 + oct(what[i + 1]) * 8 + oct(what[i + 2]);
        i += 3;
        return x;
    }
    // normal
    if (escape.find(what[i]) != escape.end()) {
        return escape[what[i++]];
    }

    throw unexpected_token("Bad escape sequence");
}

void tstream::tokenize(std::string what) {
    static std::string ext[] = {
        "+", "-", "<", ">", "=", "*", "/", "%", "!"
    };
    // Because we need to regularly access what[i + 1]
    // So we add an \0 to the end in order to prevent out-of-range access
    what.push_back('\0');
    for (int i = 0; i < what.size() - 1;) {
        if (is_in_comment) {
            if (what[i] == '*' && what[i + 1] == '/') {
                is_in_comment = false;
                i += 2;
            } else i++;
            continue;
        }

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

        // Reads an char literal, i.e. a number.
        if (what[i] == '\'') {
            tokens.push_back({ K_NUM, escaped(what, ++i) });
            if (what[i++] != '\'')
                throw unexpected_token("Char literal not properly closed");
            
            continue;
        }

        // Reads a char[] literal.
        if (what[i] == '"') {
            std::string s;
            i++;
            while (what[i] != '"' && what[i] != '\0') {
                s += escaped(what, i);
            }
            if (what[i++] != '"')
                throw unexpected_token("String literal not properly closed");
            token t = { K_STR };
            t.ident = s;
            tokens.push_back(t);
            continue;
        }

        // Reads an operator.
        std::string s = std::string() + x;
        if (what[i + 1] == '=' && std::find(ext, ext + sizeof(ext), s) != ext + sizeof(ext))
            s += what[++i];
        if (what[i] == '+' && what[i + 1] == '+' || what[i] == '-' && what[i + 1] == '-')
            s += what[++i];
        if (what[i] == '.' && what[i + 1] == '.' && i + 2 < what.size() && what[i + 2] == '.') {
            tokens.push_back({ K_DOTS });
            i += 3;
            continue;
        }

        // tokenize() only deals with a single line.
        if (what[i] == '/' && what[i + 1] == '/')
            return;
        if (what[i] == '/' && what[i + 1] == '*') {
            is_in_comment = true;
            continue;
        }
            
        tokens.push_back({ tkmap[s] });
        i++;
    }
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
#include "lexer.h"
#include "utils.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>

tstream tin;

void tstream::tokenize(const std::string& what) {
    for (int i = 0; i < what.size();) {
        char x = what[i];

        // Reads an identifier - or a keyword.
        if (isalpha(x)) {
            std::string str;
            while (isalpha(what[i]))
                str.push_back(what[i++]);
            
            if (str == "return")
                tokens.push_back({ K_RET });
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
        if (x == '+')
            tokens.push_back({ K_PLUS });
        if (x == '-')
            tokens.push_back({ K_MINUS });
        if (x == '*')
            tokens.push_back({ K_MUL });
        if (x == '/')
            tokens.push_back({ K_DIV });
        if (x == '%')
            tokens.push_back({ K_MOD });
        if (x == ';')
            tokens.push_back({ K_SEMICOLON });
        if (x == '(')
            tokens.push_back({ K_LBRACKET });
        if (x == ')')
            tokens.push_back({ K_RBRACKET });
        i++;
    }
}

void tstream::_print() {
    for (auto t : tokens)
        std::cout << fmt::format("{}: {}\n", (int) t.ty, t.val);
}

void expect(token_type t) {
    if (tin.consume().ty != t)
        throw unexpected_token();
}

bool test(token_type t) {
    if (tin.peek().ty == t) {
        tin.consume();
        return true;
    }
    return false;
}
#include "lexer.h"
#include "utils.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>

tstream tin;

void tstream::tokenize(const std::string& what) {
    for (int i = 0; i < what.size(); i++) {
        char x = what[i];

        // Reads a number.
        if (isdigit(x)) {
            int val = 0;
            for (; isdigit(what[i]); i++)
                val = 10 * val + what[i] - '0';
            i--;
            tokens.push_back({ K_NUM, val });
            continue;
        }

        // Reads an operator.
        if (x == '+')
            tokens.push_back({ K_PLUS });
        if (x == '-')
            tokens.push_back({ K_MINUS });
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

void test(token_type t) {
    if (tin.peek().ty != t)
        throw unexpected_token();
}
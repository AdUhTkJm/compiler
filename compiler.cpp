#include "assem.h"
#include "check.h"
#include <iostream>

int main() {
    for (std::string str; getline(std::cin, str); tin.tokenize(str));
    tin.seteof();

    parse();
    check();

    auto ir = generate();

    assemble(std::cout, ir);
    return 0;
}
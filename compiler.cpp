#include "assem.h"
#include "check.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs("test.c");
    std::ofstream ofs("assem.s");

    for (std::string str; getline(ifs, str); tin.tokenize(str));
    tin.seteof();

    parse();
    check();

    auto ir = generate();

    assemble(ofs, ir);
    return 0;
}
#include "assem.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs("test.c");
    std::ofstream ofs("assem.s");
    
    // For debug uses; links putchar() in stdio.h
    ofs << "extern putchar\n";

    for (std::string str; getline(ifs, str); tin.tokenize(str));
    tin.seteof();

    parse();

    auto ir = generate();

    assemble(ofs, ir);
    return 0;
}
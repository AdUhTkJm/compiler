#include "assem.h"
#include "utils.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs("test.c");
    std::ofstream ofs("assem.s");
    
    for (std::string str; ifs >> str; tin.tokenize(str));
    tin.seteof();

    try {
        parse();
    } catch (unexpected_token t) {
        std::cout << t.what() << "\n";
        std::cout << "Parsing failed.\n";
        return 0;
    }

    auto ir = generate();

    assemble(ofs, ir);
    return 0;
}
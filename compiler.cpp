#include "assem.h"
#include "utils.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs("test.txt");
    std::ofstream ofs("assem.s");
    
    for (std::string str; ifs >> str; tin.tokenize(str));
    std::cout << "Tokenising successful.\n";

    auto ast = parse();
    std::cout << "Parsing successful.\n";

    auto ir = generate(ast);
    std::cout << "Generation successful.\n";

    print_ir(ir);
    assemble(ofs, ir);
    std::cout << "Assembly successful.\n";
    return 0;
}
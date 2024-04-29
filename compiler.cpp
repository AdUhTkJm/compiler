#include "assem.h"
#include "utils.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs("test.c");
    std::ofstream ofs("assem.s");
    
    for (std::string str; ifs >> str; tin.tokenize(str));
    tin.seteof();
    std::cout << "Tokenisation successful.\n";

    try {
        parse();
    } catch (unexpected_token t) {
        std::cout << t.what() << "\n";
        std::cout << "Parsing failed.\n";
        return 0;
    }
    std::cout << "Parsing successful.\n";

    for (auto f : funcs)
        print_ast(ofs, f->body);
    ofs << "\n\n" << std::flush;

    auto ir = generate();
    std::cout << "Generation successful.\n";

    for (auto [f, i] : ir) {
        ofs << "; " << f->name << "():\n";
        print_ir(ofs, i);
    }
    ofs << "\n\n" << std::flush;

    assemble(ofs, ir);
    std::cout << "Assembly successful.\n";
    return 0;
}
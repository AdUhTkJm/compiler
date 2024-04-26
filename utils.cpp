#include "utils.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>

template<class It, class V>
bool contains(It begin, It end, V value) {
    return std::find(begin, end, value) != end;
}

void print_ir(std::vector<ir>& irs) {
    for (auto x : irs)
        switch (x.ty) {
        case I_IMM:
            std::cout << fmt::format("IMM #{} = {}\n", x.a0->ind, x.imm);
            break;
        case I_ADD:
            std::cout << fmt::format("ADD #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        case I_SUB:
            std::cout << fmt::format("SUB #{} #{}\n", x.a0->ind, x.a1->ind);
            break;
        }
}
#pragma once
#include "lexer.h"
#include "ast.h"
#include "ir.h"
#include "assem.h"

// Finds if the container defined by [begin, end)
// contains value.
template<class It, class V>
bool contains(It begin, It end, V value);

void print_ast(node*);
void print_ir(std::vector<ir>&);
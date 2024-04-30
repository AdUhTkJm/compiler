#pragma once
#include "lexer.h"
#include "ast.h"
#include "ir.h"
#include "assem.h"

void print_ast(std::ostream&, node*, int=0);
void print_ir(std::ostream&, std::vector<ir*>&);
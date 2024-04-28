#pragma once
#include "ir.h"
#include <string>
#include <fstream>

// Translate IR into x86 assembly.
void assemble(std::ostream&, decltype(generate())&);
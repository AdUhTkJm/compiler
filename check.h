#pragma once
#include "ast.h"

struct semantic_error: std::exception {
    std::string msg;
    
    const char* what() const noexcept override {
        return msg.c_str();
    }

    semantic_error(std::string msg): msg(msg) {}
};

void check();
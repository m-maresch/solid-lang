#ifndef SOLID_LANG_LEXER_H
#define SOLID_LANG_LEXER_H

#include <string>

enum Token {
    t_eof = -1,
    t_func = -2,
    t_native = -3,
    t_id = -4,
    t_num = -5,
    t_when = -6,
    t_then = -7,
    t_otherwise = -8,
    t_while = -9,
    t_for = -10,
    t_in = -11,
    t_step = -12,
    t_do = -13,
    t_unary = -14,
    t_binary = -15,
    t_operator = -16,
};

class Lexer {

public:
    int GetCurrentToken() { return CurrentToken; }

    int GetNextToken() {
        CurrentToken = GetToken();
        return CurrentToken;
    }

    std::string GetIdVal() { return IdVal; }

    double GetNumVal() { return NumVal; }

private:
    std::string IdVal;
    double NumVal;
    int LastChar = ' ';
    int CurrentToken;

    int GetToken();
};


#endif

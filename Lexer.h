#ifndef SOLID_LANG_LEXER_H
#define SOLID_LANG_LEXER_H

#include <string>

enum Token {
    token_eof = -1,
    token_func = -2,
    token_extern = -3,
    token_id = -4,
    token_num = -5,
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

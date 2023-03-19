#include "Lexer.h"

bool isDigitCharacter(char input) {
    return isdigit(input) || input == '.';
}

int Lexer::GetToken() {
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) {
        IdVal = (char) LastChar;
        LastChar = getchar();
        while (isalnum(LastChar)) {
            IdVal += (char) LastChar;
            LastChar = getchar();
        }

        if (IdVal == "func")
            return token_func;
        else if (IdVal == "extern")
            return token_extern;
        else return token_id;
    }

    if (isDigitCharacter(LastChar)) {
        std::string Num;
        do {
            Num += (char) LastChar;
            LastChar = getchar();
        } while (isDigitCharacter(LastChar));

        NumVal = (char) strtod(Num.c_str(), nullptr);
        return token_num;
    }

    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetToken();
    }

    if (LastChar == EOF)
        return token_eof;

    int Current = LastChar;
    LastChar = getchar();
    return Current;
}

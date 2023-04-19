#include "Lexer.h"

bool IsDigitCharacter(char input) {
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
        else if (IdVal == "native")
            return token_native;
        else return token_id;
    }

    if (IsDigitCharacter(LastChar)) {
        std::string Num;
        do {
            Num += (char) LastChar;
            LastChar = getchar();
        } while (IsDigitCharacter(LastChar));

        NumVal = strtod(Num.c_str(), nullptr);
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

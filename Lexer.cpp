#include "Lexer.h"

bool IsDigitCharacter(char input) {
    return isdigit(input) || input == '.';
}

int Lexer::GetToken() {
    while (isspace(LastChar))
        LastChar = getc(In);

    if (isalpha(LastChar)) {
        IdVal = (char) LastChar;
        LastChar = getc(In);
        while (isalnum(LastChar)) {
            IdVal += (char) LastChar;
            LastChar = getc(In);
        }

        if (IdVal == "func")
            return t_func;
        else if (IdVal == "native")
            return t_native;
        else if (IdVal == "when")
            return t_when;
        else if (IdVal == "then")
            return t_then;
        else if (IdVal == "otherwise")
            return t_otherwise;
        else if (IdVal == "while")
            return t_while;
        else if (IdVal == "for")
            return t_for;
        else if (IdVal == "in")
            return t_in;
        else if (IdVal == "step")
            return t_step;
        else if (IdVal == "do")
            return t_do;
        else if (IdVal == "unary")
            return t_unary;
        else if (IdVal == "binary")
            return t_binary;
        else if (IdVal == "operator")
            return t_operator;
        else return t_id;
    }

    if (IsDigitCharacter(LastChar)) {
        std::string Num;
        do {
            Num += (char) LastChar;
            LastChar = getc(In);
        } while (IsDigitCharacter(LastChar));

        NumVal = strtod(Num.c_str(), nullptr);
        return t_num;
    }

    if (LastChar == '#') {
        do {
            LastChar = getc(In);
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetToken();
    }

    if (LastChar == EOF)
        return t_eof;

    int Current = LastChar;
    LastChar = getc(In);
    return Current;
}

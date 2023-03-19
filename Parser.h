#ifndef SOLID_LANG_PARSER_H
#define SOLID_LANG_PARSER_H

#include <utility>
#include <map>

#include "Lexer.h"
#include "Expression.h"

class Parser {
public:
    explicit Parser(std::shared_ptr<Lexer> Lexer) : Lexer(std::move(Lexer)) {}

    std::unique_ptr<Expression> ParseExpression();

    std::unique_ptr<Expression> ParsePrimaryExpression();

    std::unique_ptr<Expression> ParseIdExpression();

    std::unique_ptr<Expression> ParseNumExpression();

    std::unique_ptr<Expression> ParseParenthesisExpression();

    std::unique_ptr<FunctionDeclaration> ParseFunctionDeclaration();

    std::unique_ptr<FunctionDefinition> ParseFunctionDefinition();

    std::unique_ptr<FunctionDeclaration> ParseExtern();

    std::unique_ptr<FunctionDefinition> ParseTopLevelExpression();

private:
    std::shared_ptr<Lexer> Lexer;
    std::map<char, int> BinaryOperatorPrecedences = {{'*', 40},
                                                     {'+', 20},
                                                     {'-', 20},
                                                     {'<', 10}};

    int GetTokenPrecedence();

    std::unique_ptr<Expression> ParseRightSideOfBinaryOperator(int Precedence, std::unique_ptr<Expression> LeftSide);

    template<class T>
    std::unique_ptr<T> LogError(const char *Message) {
        fprintf(stderr, "Error: %s\n", Message);
        return nullptr;
    }
};


#endif
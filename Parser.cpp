#include "Parser.h"

std::unique_ptr<Expression> Parser::ParseExpression() {
    auto LeftSide = ParsePrimaryExpression();
    if (!LeftSide)
        return nullptr;
    return ParseRightSideOfBinaryOperator(0, std::move(LeftSide));
}

std::unique_ptr<Expression> Parser::ParsePrimaryExpression() {
    switch (Lexer.GetCurrentToken()) {
        case token_id:
            return ParseIdExpression();
        case token_num:
            return ParseNumExpression();
        case '(':
            return ParseParenthesisExpression();
        default:
            return LogError<Expression>("unknown token while parsing expression");
    }
}

std::unique_ptr<Expression> Parser::ParseIdExpression() {
    auto Name = Lexer.GetIdVal();
    Lexer.GetNextToken(); // consume id

    if (Lexer.GetCurrentToken() != '(') // if it's not a function call then it's just a variable
        return std::make_unique<VariableExpression>(Name);

    Lexer.GetNextToken(); // consume '(' of function call
    std::vector<std::unique_ptr<Expression>> Arguments;
    if (Lexer.GetCurrentToken() != ')') {
        while (true) {
            auto Argument = ParseExpression();
            if (Argument) {
                Arguments.push_back(std::move(Argument));
            } else {
                return nullptr;
            }

            if (Lexer.GetCurrentToken() == ')')
                break;

            if (Lexer.GetCurrentToken() != ',')
                return LogError<Expression>("expected ')' or ','");
            Lexer.GetNextToken(); // consume ','
        }
    }
    Lexer.GetNextToken(); // consume ')'
    return std::make_unique<FunctionCall>(Name, std::move(Arguments));
}

std::unique_ptr<Expression> Parser::ParseNumExpression() {
    auto Num = std::make_unique<NumExpression>(Lexer.GetNumVal());
    Lexer.GetNextToken();  // consume number
    return std::move(Num);
}

// parse: '(' expression ')'
std::unique_ptr<Expression> Parser::ParseParenthesisExpression() {
    Lexer.GetNextToken(); // consume '('
    auto Value = ParseExpression();
    if (!Value)
        return nullptr;
    if (Lexer.GetCurrentToken() != ')')
        return LogError<Expression>("expected ')'");
    Lexer.GetNextToken(); // consume ')'
    return Value;
}

int Parser::GetTokenPrecedence() {
    if (!isascii(Lexer.GetCurrentToken()))
        return -1;

    int Precedence = BinaryOperatorPrecedences[(char) Lexer.GetCurrentToken()];
    if (Precedence <= 0)
        return -1;
    return Precedence;
}

std::unique_ptr<Expression> Parser::ParseRightSideOfBinaryOperator(int ExpressionPrecedence,
                                                                   std::unique_ptr<Expression> LeftSide) {
    while (true) {
        int TokenPrecedence = GetTokenPrecedence();

        if (TokenPrecedence < ExpressionPrecedence)
            return LeftSide;

        int BinaryOperator = Lexer.GetCurrentToken();
        Lexer.GetNextToken(); // consume binary operator

        auto RightSide = ParsePrimaryExpression();
        if (!RightSide)
            return nullptr;

        int NextPrecedence = GetTokenPrecedence();
        if (TokenPrecedence < NextPrecedence) {
            RightSide = ParseRightSideOfBinaryOperator(
                    TokenPrecedence + 1,
                    std::move(RightSide)
            );
            if (!RightSide)
                return nullptr;
        }

        LeftSide = std::make_unique<BinaryExpression>(
                BinaryOperator,
                std::move(LeftSide),
                std::move(RightSide)
        );
    }
}

std::unique_ptr<FunctionDeclaration> Parser::ParseFunctionDeclaration() {
    if (Lexer.GetCurrentToken() != token_id)
        return LogError<FunctionDeclaration>("expected function name in declaration");

    auto Name = Lexer.GetIdVal();
    Lexer.GetNextToken();

    if (Lexer.GetCurrentToken() != '(')
        return LogError<FunctionDeclaration>("expected '('");


    std::vector<std::string> ArgumentNames;
    while (Lexer.GetNextToken() == token_id)
        ArgumentNames.push_back(Lexer.GetIdVal());

    if (Lexer.GetCurrentToken() != ')')
        return LogError<FunctionDeclaration>("expected ')'");

    Lexer.GetNextToken(); // consume ')'
    return std::make_unique<FunctionDeclaration>(Name, std::move(ArgumentNames));
}

std::unique_ptr<FunctionDefinition> Parser::ParseFunctionDefinition() {
    Lexer.GetNextToken(); // consume 'func'
    auto Declaration = ParseFunctionDeclaration();
    if (!Declaration)
        return nullptr;

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    return std::make_unique<FunctionDefinition>(std::move(Declaration), std::move(Body));
}

std::unique_ptr<FunctionDeclaration> Parser::ParseExtern() {
    Lexer.GetNextToken(); // consume 'extern'
    return ParseFunctionDeclaration();
}

std::unique_ptr<FunctionDefinition> Parser::ParseTopLevelExpression() {
    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    auto Declaration = std::make_unique<FunctionDeclaration>("", std::vector<std::string>());
    return std::make_unique<FunctionDefinition>(std::move(Declaration), std::move(Body));
}

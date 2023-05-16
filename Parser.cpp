#include "Parser.h"

std::unique_ptr<Expression> Parser::ParseExpression() {
    auto LeftSide = ParseUnaryExpression();
    if (!LeftSide)
        return nullptr;
    return ParseRightSideOfBinaryOperator(0, std::move(LeftSide));
}

std::unique_ptr<Expression> Parser::ParsePrimaryExpression() {
    switch (Lexer.GetCurrentToken()) {
        case t_id:
            return ParseIdExpression();
        case t_num:
            return ParseNumExpression();
        case '(':
            return ParseParenthesisExpression();
        case t_when:
            return ParseConditionalExpression();
        case t_while:
            return ParseLoopExpression();
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

std::unique_ptr<Expression> Parser::ParseUnaryExpression() {
    int CurrentToken = Lexer.GetCurrentToken();
    if (CurrentToken == '(' || CurrentToken == ',' || !isascii(CurrentToken)) {
        return ParsePrimaryExpression();
    }

    int Operator = CurrentToken;
    Lexer.GetNextToken(); // consume operator
    if (auto Operand = ParseUnaryExpression())
        return std::make_unique<UnaryExpression>(Operator, std::move(Operand));
    return nullptr;
}

/// parse: 'when' expression 'then' expression 'otherwise' expression
std::unique_ptr<Expression> Parser::ParseConditionalExpression() {
    Lexer.GetNextToken(); // consume 'when'

    auto Condition = ParseExpression();
    if (!Condition) {
        return nullptr;
    }

    if (Lexer.GetCurrentToken() != t_then)
        return LogError<Expression>("expected 'then'");
    Lexer.GetNextToken(); // consume 'then'

    auto Then = ParseExpression();
    if (!Then) {
        return nullptr;
    }

    if (Lexer.GetCurrentToken() != t_otherwise)
        return LogError<Expression>("expected 'otherwise'");
    Lexer.GetNextToken(); // consume 'otherwise'

    auto Otherwise = ParseExpression();
    if (!Otherwise) {
        return nullptr;
    }

    return std::make_unique<ConditionalExpression>(std::move(Condition), std::move(Then), std::move(Otherwise));
}

/// parse: 'while' expr 'for' id '=' expr ('step' expr)? 'do' expression
std::unique_ptr<Expression> Parser::ParseLoopExpression() {
    Lexer.GetNextToken(); // consume 'while'

    auto While = ParseExpression();
    if (!While) {
        return nullptr;
    }

    if (Lexer.GetCurrentToken() != t_for)
        return LogError<Expression>("expected 'for'");
    Lexer.GetNextToken(); // consume 'for'

    if (Lexer.GetCurrentToken() != t_id)
        return LogError<Expression>("expected id");

    std::string Name = Lexer.GetIdVal();
    Lexer.GetNextToken(); // consume id

    if (Lexer.GetCurrentToken() != '=')
        return LogError<Expression>("expected '='");
    Lexer.GetNextToken(); // consume '='

    auto For = ParseExpression();
    if (!For) {
        return nullptr;
    }

    // optional step
    std::unique_ptr<Expression> Step;
    if (Lexer.GetCurrentToken() == t_step) {
        Lexer.GetNextToken(); // consume 'step'
        Step = ParseExpression();
        if (!Step) {
            return nullptr;
        }
    }

    if (Lexer.GetCurrentToken() != t_do)
        return LogError<Expression>("expected 'do'");
    Lexer.GetNextToken(); // consume 'do'

    auto Body = ParseExpression();
    if (!Body) {
        return nullptr;
    }

    return std::make_unique<LoopExpression>(Name, std::move(For), std::move(While), std::move(Step), std::move(Body));
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

        auto RightSide = ParseUnaryExpression();
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
    std::string Name;
    int Type; // 0 = id, 1 = unary, 2 = binary

    switch (Lexer.GetCurrentToken()) {
        default:
            return LogError<FunctionDeclaration>("expected function name in declaration");
        case t_id:
            Name = Lexer.GetIdVal();
            Type = 0;

            Lexer.GetNextToken(); // consume id
            break;
        case t_unary:
            Lexer.GetNextToken(); // consume 'unary'

            if (!isascii(Lexer.GetCurrentToken()))
                return LogError<FunctionDeclaration>("expected unary operator");

            Name = std::string("unary") + ((char) Lexer.GetCurrentToken());
            Type = 1;

            Lexer.GetNextToken(); // consume operator symbol
            break;
        case t_binary:
            Lexer.GetNextToken(); // consume 'binary'

            if (!isascii(Lexer.GetCurrentToken()))
                return LogError<FunctionDeclaration>("expected binary operator");

            char Operator = (char) Lexer.GetCurrentToken();
            Name = std::string("binary") + Operator;
            Type = 2;
            int Precedence = 30;

            Lexer.GetNextToken(); // consume operator symbol

            // optional operator precedence
            if (Lexer.GetCurrentToken() == t_num) {
                double Num = Lexer.GetNumVal();
                if (Num > 100 || Num < 1)
                    return LogError<FunctionDeclaration>("Precedence must be in range 1..100");

                Precedence = (int) Num;

                Lexer.GetNextToken(); // consume operator precedence
            }

            BinaryOperatorPrecedences[Operator] = Precedence;
            break;
    }

    if (Lexer.GetCurrentToken() != '(')
        return LogError<FunctionDeclaration>("expected '('");

    std::vector<std::string> ArgumentNames;
    while (Lexer.GetNextToken() == t_id)
        ArgumentNames.push_back(Lexer.GetIdVal());

    if (Lexer.GetCurrentToken() != ')')
        return LogError<FunctionDeclaration>("expected ')'");

    Lexer.GetNextToken(); // consume ')'

    if ((Type == 1 && ArgumentNames.size() != 1) || (Type == 2 && ArgumentNames.size() != 2))
        return LogError<FunctionDeclaration>("invalid number of operands for unary/binary operator");

    return std::make_unique<FunctionDeclaration>(Name, std::move(ArgumentNames));
}

std::unique_ptr<FunctionDefinition> Parser::ParseFunctionDefinition() {
    Lexer.GetNextToken(); // consume 'func'/'operator'
    auto Declaration = ParseFunctionDeclaration();
    if (!Declaration)
        return nullptr;

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    return std::make_unique<FunctionDefinition>(std::move(Declaration), std::move(Body));
}

std::unique_ptr<FunctionDeclaration> Parser::ParseNative() {
    Lexer.GetNextToken(); // consume 'native'
    return ParseFunctionDeclaration();
}

std::unique_ptr<FunctionDefinition> Parser::ParseTopLevelExpression() {
    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    auto Declaration = std::make_unique<FunctionDeclaration>("__anonymous_top_level_expr", std::vector<std::string>());
    return std::make_unique<FunctionDefinition>(std::move(Declaration), std::move(Body));
}

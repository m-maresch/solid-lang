#ifndef SOLID_LANG_EXPRESSION_H
#define SOLID_LANG_EXPRESSION_H

#include <string>
#include <utility>
#include <vector>
#include "Visitor.h"

class Expression {

public:
    virtual ~Expression() = default;

    virtual void Accept(Visitor &visitor) = 0;
};

class VariableExpression : public Expression {
    std::string Name;

public:
    VariableExpression(std::string Name) : Name(std::move(Name)) {}

    void Accept(Visitor &visitor) override;

    std::string GetName() {
        return Name;
    }
};

class FunctionCall : public Expression {
    std::string Function;
    std::vector<std::unique_ptr<Expression>> Arguments;

public:
    FunctionCall(std::string Function, std::vector<std::unique_ptr<Expression>> Arguments)
            : Function(std::move(Function)), Arguments(std::move(Arguments)) {}

    void Accept(Visitor &visitor) override;

    std::string GetFunction() {
        return Function;
    }

    std::vector<std::unique_ptr<Expression>> &GetArguments() {
        return Arguments;
    }
};

class FunctionDeclaration : public Expression {
    std::string Function;
    std::vector<std::string> Arguments;

public:
    FunctionDeclaration(std::string Function, std::vector<std::string> Arguments)
            : Function(std::move(Function)), Arguments(std::move(Arguments)) {}

    void Accept(Visitor &visitor) override;

    std::string GetFunction() {
        return Function;
    }

    std::vector<std::string> GetArguments() {
        return Arguments;
    }
};

class FunctionDefinition : public Expression {
    std::unique_ptr<FunctionDeclaration> Declaration;
    std::unique_ptr<Expression> Implementation;

public:
    FunctionDefinition(std::unique_ptr<FunctionDeclaration> Declaration,
                       std::unique_ptr<Expression> Implementation)
            : Declaration(std::move(Declaration)), Implementation(std::move(Implementation)) {}

    void Accept(Visitor &visitor) override;

    FunctionDeclaration &GetDeclaration() {
        return *Declaration;
    }

    Expression &GetImplementation() {
        return *Implementation;
    }
};

class BinaryExpression : public Expression {
    char Operator;
    std::unique_ptr<Expression> LeftSide;
    std::unique_ptr<Expression> RightSide;

public:
    BinaryExpression(char Operator, std::unique_ptr<Expression> LeftSide, std::unique_ptr<Expression> RightSide)
            : Operator(Operator), LeftSide(std::move(LeftSide)), RightSide(std::move(RightSide)) {}

    void Accept(Visitor &visitor) override;

    char GetOperator() {
        return Operator;
    }

    Expression &GetLeftSide() {
        return *LeftSide;
    }

    Expression &GetRightSide() {
        return *RightSide;
    }
};

class NumExpression : public Expression {
    double Val;

public:
    NumExpression(double Val) : Val(Val) {}

    void Accept(Visitor &visitor) override;

    double GetVal() {
        return Val;
    }
};

#endif

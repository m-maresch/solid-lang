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
    std::string Name;
    std::vector<std::unique_ptr<Expression>> Arguments;

public:
    FunctionCall(std::string Name, std::vector<std::unique_ptr<Expression>> Arguments)
            : Name(std::move(Name)), Arguments(std::move(Arguments)) {}

    void Accept(Visitor &visitor) override;

    std::string GetName() {
        return Name;
    }

    std::vector<std::unique_ptr<Expression>> &GetArguments() {
        return Arguments;
    }
};

class FunctionDeclaration : public Expression {
    std::string Name;
    std::vector<std::string> Arguments;

public:
    FunctionDeclaration(std::string Name, std::vector<std::string> Arguments)
            : Name(std::move(Name)), Arguments(std::move(Arguments)) {}

    void Accept(Visitor &visitor) override;

    std::string GetName() {
        return Name;
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

    std::unique_ptr<FunctionDeclaration> TakeDeclaration() {
        return std::move(Declaration);
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

class ConditionalExpression : public Expression {
    std::unique_ptr<Expression> Condition;
    std::unique_ptr<Expression> Then;
    std::unique_ptr<Expression> Otherwise;

public:
    ConditionalExpression(std::unique_ptr<Expression> Condition, std::unique_ptr<Expression> Then,
                          std::unique_ptr<Expression> Otherwise)
            : Condition(std::move(Condition)), Then(std::move(Then)), Otherwise(std::move(Otherwise)) {}

    void Accept(Visitor &visitor) override;

    Expression &GetCondition() {
        return *Condition;
    }

    Expression &GetThen() {
        return *Then;
    }

    Expression &GetOtherwise() {
        return *Otherwise;
    }
};

class LoopExpression : public Expression {
    std::string VariableName;
    std::unique_ptr<Expression> For;
    std::unique_ptr<Expression> While;
    std::unique_ptr<Expression> Step;
    std::unique_ptr<Expression> Body;

public:
    LoopExpression(std::string VariableName, std::unique_ptr<Expression> For,
                   std::unique_ptr<Expression> While, std::unique_ptr<Expression> Step,
                   std::unique_ptr<Expression> Body)
            : VariableName(std::move(VariableName)), For(std::move(For)), While(std::move(While)),
              Step(std::move(Step)), Body(std::move(Body)) {}

    void Accept(Visitor &visitor) override;

    std::string GetVariableName() {
        return VariableName;
    }

    Expression &GetFor() {
        return *For;
    }

    Expression &GetWhile() {
        return *While;
    }

    Expression &GetStep() {
        return *Step;
    }

    Expression &GetBody() {
        return *Body;
    }

    bool HasStep() {
        return static_cast<bool>(Step);
    }
};

#endif

#ifndef SOLID_LANG_EXPRESSIONVISITOR_H
#define SOLID_LANG_EXPRESSIONVISITOR_H

class VariableExpression;

class VariableDefinition;

class FunctionCall;

class FunctionDeclaration;

class FunctionDefinition;

class UnaryExpression;

class BinaryExpression;

class NumExpression;

class ConditionalExpression;

class LoopExpression;

class ExpressionVisitor {

public:
    virtual ~ExpressionVisitor() = default;

    virtual void Visit(VariableExpression &Expression) = 0;

    virtual void Visit(VariableDefinition &Expression) = 0;

    virtual void Visit(FunctionCall &Expression) = 0;

    virtual void Visit(FunctionDeclaration &Expression) = 0;

    virtual void Visit(FunctionDefinition &Expression) = 0;

    virtual void Visit(UnaryExpression &Expression) = 0;

    virtual void Visit(BinaryExpression &Expression) = 0;

    virtual void Visit(NumExpression &Expression) = 0;

    virtual void Visit(ConditionalExpression &Expression) = 0;

    virtual void Visit(LoopExpression &Expression) = 0;

    virtual void Register(std::unique_ptr<FunctionDeclaration> Declaration) = 0;
};

#endif

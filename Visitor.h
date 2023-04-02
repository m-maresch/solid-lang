#ifndef SOLID_LANG_VISITOR_H
#define SOLID_LANG_VISITOR_H

class VariableExpression;

class FunctionCall;

class FunctionDeclaration;

class FunctionDefinition;

class BinaryExpression;

class NumExpression;

class Visitor {

public:
    virtual ~Visitor() = default;

    virtual void Visit(VariableExpression &expression) = 0;

    virtual void Visit(FunctionCall &expression) = 0;

    virtual void Visit(FunctionDeclaration &expression) = 0;

    virtual void Visit(FunctionDefinition &expression) = 0;

    virtual void Visit(BinaryExpression &expression) = 0;

    virtual void Visit(NumExpression &expression) = 0;
};

#endif
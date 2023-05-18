#include "Expression.h"

void VariableExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void VariableDefinition::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void FunctionCall::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void FunctionDeclaration::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void FunctionDefinition::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void UnaryExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void BinaryExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void NumExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void ConditionalExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

void LoopExpression::Accept(ExpressionVisitor &Visitor) {
    Visitor.Visit(*this);
}

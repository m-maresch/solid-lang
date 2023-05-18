#include "Expression.h"

void VariableExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void VariableDefinition::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void FunctionCall::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void FunctionDeclaration::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void FunctionDefinition::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void UnaryExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void BinaryExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void NumExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void ConditionalExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

void LoopExpression::Accept(ExpressionVisitor &visitor) {
    visitor.Visit(*this);
}

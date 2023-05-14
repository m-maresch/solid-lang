#include "Expression.h"

void VariableExpression::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void FunctionCall::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void FunctionDeclaration::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void FunctionDefinition::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void BinaryExpression::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void NumExpression::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void ConditionalExpression::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

void LoopExpression::Accept(Visitor &visitor) {
    visitor.Visit(*this);
}

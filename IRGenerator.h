#ifndef SOLID_LANG_IRGENERATOR_H
#define SOLID_LANG_IRGENERATOR_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <map>

#include "Visitor.h"

using namespace llvm;

class IRGenerator : public Visitor {
    LLVMContext &Context;

    IRBuilder<> &Builder;

    Module &Module;

    std::map<std::string, Value *> ValuesByName;

    Value *Current;

public:
    explicit IRGenerator(LLVMContext &Context, IRBuilder<> &Builder, class Module &Module)
            : Context(Context), Builder(Builder), Module(Module) {}

    void Visit(VariableExpression &expression) override;

    void Visit(FunctionCall &expression) override;

    void Visit(FunctionDeclaration &expression) override;

    void Visit(FunctionDefinition &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;

    Value *GetValue() {
        return Current;
    }

    void LogError(const char *Message) {
        fprintf(stderr, "Error: %s\n", Message);
    }
};

class IRPrinter : public Visitor {
    IRGenerator &IRGenerator;

    void Print() {
        if (auto *IR = IRGenerator.GetValue()) {
            IR->print(errs());
            fprintf(stderr, "\n");
        }
    }

public:
    explicit IRPrinter(class IRGenerator &IRGenerator) : IRGenerator(IRGenerator) {}

    void Visit(VariableExpression &expression) override;

    void Visit(FunctionCall &expression) override;

    void Visit(FunctionDeclaration &expression) override;

    void Visit(FunctionDefinition &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;
};


#endif

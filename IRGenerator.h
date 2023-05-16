#ifndef SOLID_LANG_IRGENERATOR_H
#define SOLID_LANG_IRGENERATOR_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/IR/LegacyPassManager.h>
#include <map>
#include <utility>

#include "Visitor.h"

using namespace llvm;

class IRGenerator : public Visitor {
    LLVMContext &Context;

    IRBuilder<> &Builder;

    Module &Module;

    legacy::FunctionPassManager &PassManager;

    std::map<std::string, Value *> &ValuesByName;

    std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations;

    Value *Current;

    Function *LookupFunction(std::string Name);

public:
    explicit IRGenerator(LLVMContext &Context, IRBuilder<> &Builder, class Module &Module,
                         legacy::FunctionPassManager &PassManager, std::map<std::string, Value *> &ValuesByName,
                         std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations)
            : Context(Context), Builder(Builder), Module(Module), PassManager(PassManager), ValuesByName(ValuesByName),
              FunctionDeclarations(FunctionDeclarations) {}

    void Visit(VariableExpression &expression) override;

    void Visit(FunctionCall &expression) override;

    void Visit(FunctionDeclaration &expression) override;

    void Visit(FunctionDefinition &expression) override;

    void Visit(UnaryExpression &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;

    void Visit(ConditionalExpression &expression) override;

    void Visit(LoopExpression &expression) override;

    Value *GetValue() {
        return Current;
    }

    void LogError(const char *Message) {
        fprintf(stderr, "Error: %s\n", Message);
    }

    void Register(std::unique_ptr<FunctionDeclaration> Declaration);
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

    void Visit(UnaryExpression &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;

    void Visit(ConditionalExpression &expression) override;

    void Visit(LoopExpression &expression) override;
};


#endif

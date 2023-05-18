#ifndef SOLID_LANG_IRGENERATOR_H
#define SOLID_LANG_IRGENERATOR_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/IR/LegacyPassManager.h>
#include <map>
#include <utility>

#include "ExpressionVisitor.h"

using namespace llvm;

class IRGenerator : public ExpressionVisitor {
    LLVMContext &Context;

    IRBuilder<> &Builder;

    Module &Module;

    legacy::FunctionPassManager &PassManager;

    std::map<std::string, AllocaInst *> &ValuesByName;

    std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations;

    Value *Current;

    Function *LookupFunction(std::string Name);

    AllocaInst *CreateAlloca(Function *Func, StringRef Name);

public:
    explicit IRGenerator(LLVMContext &Context, IRBuilder<> &Builder, class Module &Module,
                         legacy::FunctionPassManager &PassManager, std::map<std::string, AllocaInst *> &ValuesByName,
                         std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations)
            : Context(Context), Builder(Builder), Module(Module), PassManager(PassManager), ValuesByName(ValuesByName),
              FunctionDeclarations(FunctionDeclarations) {}

    void Visit(VariableExpression &expression) override;

    void Visit(VariableDefinition &expression) override;

    void Visit(FunctionCall &expression) override;

    void Visit(FunctionDeclaration &expression) override;

    void Visit(FunctionDefinition &expression) override;

    void Visit(UnaryExpression &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;

    void Visit(ConditionalExpression &expression) override;

    void Visit(LoopExpression &expression) override;

    void Register(std::unique_ptr<FunctionDeclaration> Declaration) override;

    Value *GetValue() {
        return Current;
    }

    void LogError(const char *Message) {
        fprintf(stderr, "Error: %s\n", Message);
    }
};

class IRPrinter : public ExpressionVisitor {
    std::unique_ptr<class IRGenerator> IRGenerator;

    void Print() {
        if (auto *IR = IRGenerator->GetValue()) {
            IR->print(errs());
            fprintf(stderr, "\n");
        }
    }

public:
    explicit IRPrinter(std::unique_ptr<class IRGenerator> IRGenerator) : IRGenerator(std::move(IRGenerator)) {}

    void Visit(VariableExpression &expression) override;

    void Visit(VariableDefinition &expression) override;

    void Visit(FunctionCall &expression) override;

    void Visit(FunctionDeclaration &expression) override;

    void Visit(FunctionDefinition &expression) override;

    void Visit(UnaryExpression &expression) override;

    void Visit(BinaryExpression &expression) override;

    void Visit(NumExpression &expression) override;

    void Visit(ConditionalExpression &expression) override;

    void Visit(LoopExpression &expression) override;

    void Register(std::unique_ptr<FunctionDeclaration> Declaration) override;
};


#endif

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

    std::unique_ptr<legacy::FunctionPassManager> PassManager;

    std::map<std::string, AllocaInst *> &ValuesByName;

    std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations;

    Value *Current;

    Function *LookupFunction(std::string Name);

    AllocaInst *CreateAlloca(Function *Func, StringRef Name);

public:
    explicit IRGenerator(LLVMContext &Context, IRBuilder<> &Builder, class Module &Module,
                         std::unique_ptr<legacy::FunctionPassManager> PassManager,
                         std::map<std::string, AllocaInst *> &ValuesByName,
                         std::map<std::string, std::unique_ptr<FunctionDeclaration>> &FunctionDeclarations)
            : Context(Context), Builder(Builder), Module(Module), PassManager(std::move(PassManager)),
              ValuesByName(ValuesByName), FunctionDeclarations(FunctionDeclarations) {}

    void Visit(VariableExpression &Expression) override;

    void Visit(VariableDefinition &Expression) override;

    void Visit(FunctionCall &Expression) override;

    void Visit(FunctionDeclaration &Expression) override;

    void Visit(FunctionDefinition &Expression) override;

    void Visit(UnaryExpression &Expression) override;

    void Visit(BinaryExpression &Expression) override;

    void Visit(NumExpression &Expression) override;

    void Visit(ConditionalExpression &Expression) override;

    void Visit(LoopExpression &Expression) override;

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
            errs() << "\n" << "Generated LLVM IR:" << "\n";
            IR->print(errs());
            errs() << "\n";
        }
    }

public:
    explicit IRPrinter(std::unique_ptr<class IRGenerator> IRGenerator) : IRGenerator(std::move(IRGenerator)) {}

    void Visit(VariableExpression &Expression) override;

    void Visit(VariableDefinition &Expression) override;

    void Visit(FunctionCall &Expression) override;

    void Visit(FunctionDeclaration &Expression) override;

    void Visit(FunctionDefinition &Expression) override;

    void Visit(UnaryExpression &Expression) override;

    void Visit(BinaryExpression &Expression) override;

    void Visit(NumExpression &Expression) override;

    void Visit(ConditionalExpression &Expression) override;

    void Visit(LoopExpression &Expression) override;

    void Register(std::unique_ptr<FunctionDeclaration> Declaration) override;
};


#endif

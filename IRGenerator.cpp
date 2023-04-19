#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include "IRGenerator.h"
#include "Expression.h"

Function *IRGenerator::LookupFunction(std::string Name) {
    auto *ModuleFunction = Module.getFunction(Name);
    if (ModuleFunction) {
        return ModuleFunction;
    }

    auto Declaration = FunctionDeclarations.find(Name);
    if (Declaration != FunctionDeclarations.end()) {
        Declaration->second->Accept(*this);
        return static_cast<class Function *>(Current);
    }

    return nullptr;
}

void IRGenerator::Visit(VariableExpression &expression) {
    Value *Val = ValuesByName[expression.GetName()];
    if (!Val) {
        LogError("Variable unknown");
    }
    Current = Val;
}

void IRGenerator::Visit(FunctionCall &expression) {
    Function *Function = LookupFunction(expression.GetName());
    if (!Function) {
        LogError("Calling unknown function");
        return;
    }

    unsigned n = expression.GetArguments().size();
    if (Function->arg_size() != n) {
        LogError("Invalid number of arguments passed to function");
        return;
    }

    std::vector<Value *> ArgumentValues;
    for (unsigned i = 0; i < n; ++i) {
        expression.GetArguments()[i]->Accept(*this);
        ArgumentValues.push_back(Current);
        if (!ArgumentValues.back()) {
            Current = nullptr;
            return;
        }
    }

    Current = Builder.CreateCall(Function, ArgumentValues, "calltmp");
}

void IRGenerator::Visit(FunctionDeclaration &expression) {
    unsigned n = expression.GetArguments().size();
    std::vector<Type *> Doubles(n, Type::getDoubleTy(Context));

    FunctionType *FuncType = FunctionType::get(Type::getDoubleTy(Context), Doubles, false);

    Function *Func = Function::Create(FuncType, Function::ExternalLinkage, expression.GetName(), Module);

    unsigned i = 0;
    for (auto &Argument: Func->args()) {
        Argument.setName(expression.GetArguments()[i++]);
    }

    Current = Func;
}

void IRGenerator::Visit(FunctionDefinition &expression) {
    auto Declaration = expression.TakeDeclaration();
    auto Name = Declaration->GetName();
    FunctionDeclarations[Name] = std::move(Declaration);
    Function *Func = LookupFunction(Name);

    if (!Func) {
        Current = nullptr;
        return;
    }

    BasicBlock *Block = BasicBlock::Create(Context, "entry", Func);
    Builder.SetInsertPoint(Block);

    ValuesByName.clear();
    for (auto &Argument: Func->args()) {
        ValuesByName[std::string(Argument.getName())] = &Argument;
    }

    expression.GetImplementation().Accept(*this);
    if (Value *ReturnValue = Current) {
        Builder.CreateRet(ReturnValue);

        verifyFunction(*Func);

        PassManager.run(*Func);

        Current = Func;
        return;
    }

    Func->eraseFromParent();
    Current = nullptr;
}

void IRGenerator::Visit(BinaryExpression &expression) {
    expression.GetLeftSide().Accept(*this);
    Value *LeftSide = Current;
    expression.GetRightSide().Accept(*this);
    Value *RightSide = Current;

    if (!LeftSide || !RightSide) {
        Current = nullptr;
        return;
    }

    switch (expression.GetOperator()) {
        case '+':
            Current = Builder.CreateFAdd(LeftSide, RightSide, "addtmp");
            return;
        case '-':
            Current = Builder.CreateFSub(LeftSide, RightSide, "subtmp");
            return;
        case '*':
            Current = Builder.CreateFMul(LeftSide, RightSide, "multmp");
            return;
        case '<':
            LeftSide = Builder.CreateFCmpULT(LeftSide, RightSide, "cmptmp");
            Current = Builder.CreateUIToFP(LeftSide, Type::getDoubleTy(Context), "booltmp");
            return;
        default:
            LogError("Binary operator invalid");
            return;
    }
}

void IRGenerator::Visit(NumExpression &expression) {
    Current = ConstantFP::get(Context, APFloat(expression.GetVal()));
}

void IRGenerator::Register(std::unique_ptr<FunctionDeclaration> Declaration) {
    FunctionDeclarations[Declaration->GetName()] = std::move(Declaration);
}

void IRPrinter::Visit(VariableExpression &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionCall &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionDeclaration &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionDefinition &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(BinaryExpression &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(NumExpression &expression) {
    IRGenerator.Visit(expression);
    Print();
}

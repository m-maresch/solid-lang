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

void IRGenerator::Visit(ConditionalExpression &expression) {
    expression.GetCondition().Accept(*this);
    Value *Condition = Current;
    if (!Condition) {
        Current = nullptr;
        return;
    }

    // convert to bool (compare non-equal to 0)
    Condition = Builder.CreateFCmpONE(
            Condition, ConstantFP::get(Context, APFloat(0.0)), "whencond");

    Function *Func = Builder.GetInsertBlock()->getParent();

    BasicBlock *ThenBlock = BasicBlock::Create(Context, "then", Func);
    BasicBlock *OtherwiseBlock = BasicBlock::Create(Context, "otherwise");
    BasicBlock *MergeBlock = BasicBlock::Create(Context, "whencont");

    Builder.CreateCondBr(Condition, ThenBlock, OtherwiseBlock);

    // emit then:
    Builder.SetInsertPoint(ThenBlock);

    expression.GetThen().Accept(*this);
    Value *Then = Current;
    if (!Then) {
        Current = nullptr;
        return;
    }

    Builder.CreateBr(MergeBlock);
    ThenBlock = Builder.GetInsertBlock();

    // emit otherwise:
    Func->insert(Func->end(), OtherwiseBlock);
    Builder.SetInsertPoint(OtherwiseBlock);

    expression.GetOtherwise().Accept(*this);
    Value *Otherwise = Current;
    if (!Otherwise) {
        Current = nullptr;
        return;
    }

    Builder.CreateBr(MergeBlock);
    OtherwiseBlock = Builder.GetInsertBlock();

    // emit merge:
    Func->insert(Func->end(), MergeBlock);
    Builder.SetInsertPoint(MergeBlock);

    PHINode *PHI = Builder.CreatePHI(Type::getDoubleTy(Context), 2, "whentmp");
    PHI->addIncoming(Then, ThenBlock);
    PHI->addIncoming(Otherwise, OtherwiseBlock);

    Current = PHI;
}

void IRGenerator::Visit(LoopExpression &expression) {
    std::string VariableName = expression.GetVariableName();

    // emit for:
    expression.GetFor().Accept(*this);
    Value *For = Current;
    if (!For) {
        Current = nullptr;
        return;
    }

    Function *Func = Builder.GetInsertBlock()->getParent();
    BasicBlock *BeforeBlock = Builder.GetInsertBlock();
    BasicBlock *LoopBlock = BasicBlock::Create(Context, "loop", Func);

    // fall through from current block to loop block
    Builder.CreateBr(LoopBlock);

    Builder.SetInsertPoint(LoopBlock);

    PHINode *Variable = Builder.CreatePHI(Type::getDoubleTy(Context), 2, VariableName);
    Variable->addIncoming(For, BeforeBlock);

    Value *OriginalValue = ValuesByName[VariableName];
    ValuesByName[VariableName] = Variable;

    // emit loop body:
    expression.GetBody().Accept(*this);
    if (!Current) {
        Current = nullptr;
        return;
    }

    // emit step:
    Value *Step;
    if (expression.HasStep()) {
        expression.GetStep().Accept(*this);
        Step = Current;
        if (!Step) {
            Current = nullptr;
            return;
        }
    } else {
        // use 1
        Step = ConstantFP::get(Context, APFloat(1.0));
    }

    Value *NextVariable = Builder.CreateFAdd(Variable, Step, "nextvar");

    // emit while:
    expression.GetWhile().Accept(*this);
    Value *While = Current;
    if (!While) {
        Current = nullptr;
        return;
    }

    // convert to bool (compare non-equal to 0)
    While = Builder.CreateFCmpONE(While, ConstantFP::get(Context, APFloat(0.0)), "loopcond");

    BasicBlock *LoopEndBlock = Builder.GetInsertBlock();
    BasicBlock *AfterBlock = BasicBlock::Create(Context, "afterloop", Func);

    Builder.CreateCondBr(While, LoopBlock, AfterBlock);
    Builder.SetInsertPoint(AfterBlock);

    Variable->addIncoming(NextVariable, LoopEndBlock);

    if (OriginalValue) {
        ValuesByName[VariableName] = OriginalValue;
    } else {
        ValuesByName.erase(VariableName);
    }

    // return 0 always
    Current = Constant::getNullValue(Type::getDoubleTy(Context));
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

void IRPrinter::Visit(ConditionalExpression &expression) {
    IRGenerator.Visit(expression);
    Print();
}

void IRPrinter::Visit(LoopExpression &expression) {
    IRGenerator.Visit(expression);
    Print();
}

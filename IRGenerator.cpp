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

AllocaInst *IRGenerator::CreateAlloca(Function *Func, StringRef Name) {
    IRBuilder<> TmpBuilder(&Func->getEntryBlock(), Func->getEntryBlock().begin());
    return TmpBuilder.CreateAlloca(Type::getDoubleTy(Context), nullptr, Name);
}

void IRGenerator::Visit(VariableExpression &expression) {
    AllocaInst *Alloca = ValuesByName[expression.GetName()];
    if (!Alloca) {
        LogError("Variable unknown");
    }

    Current = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca, expression.GetName().c_str());
}

void IRGenerator::Visit(VariableDefinition &expression) {
    std::vector<AllocaInst *> OriginalValues;
    Function *Func = Builder.GetInsertBlock()->getParent();

    for (auto &Variable: expression.GetVariables()) {
        const std::string &VariableName = Variable.first;
        Expression *VariableInitializer = Variable.second.get();

        Value *Initializer;
        if (VariableInitializer) {
            VariableInitializer->Accept(*this);
            Initializer = Current;
            if (!Initializer) {
                Current = nullptr;
                return;
            }
        } else {
            // use 0
            Initializer = ConstantFP::get(Context, APFloat(0.0));
        }

        AllocaInst *Alloca = CreateAlloca(Func, VariableName);
        Builder.CreateStore(Initializer, Alloca);

        OriginalValues.push_back(ValuesByName[VariableName]);

        ValuesByName[VariableName] = Alloca;
    }

    expression.GetBody().Accept(*this);
    Value *Body = Current;
    if (!Body) {
        Current = nullptr;
        return;
    }

    unsigned n = expression.GetVariables().size();
    for (unsigned i = 0; i < n; ++i) {
        ValuesByName[expression.GetVariables()[i].first] = OriginalValues[i];
    }

    Current = Body;
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
        AllocaInst *Alloca = CreateAlloca(Func, Argument.getName());
        Builder.CreateStore(&Argument, Alloca);
        ValuesByName[std::string(Argument.getName())] = Alloca;
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

void IRGenerator::Visit(UnaryExpression &expression) {
    expression.GetOperand().Accept(*this);
    Value *Operand = Current;
    if (!Operand) {
        Current = nullptr;
        return;
    }

    Function *Func = LookupFunction(std::string("unary") + expression.GetOperator());
    if (!Func) {
        LogError("Unknown unary operator");
        Current = nullptr;
        return;
    }

    Current = Builder.CreateCall(Func, Operand, "unop");
}

void IRGenerator::Visit(BinaryExpression &expression) {
    if (expression.GetOperator() == '=') {
        auto &LeftSide = dynamic_cast<VariableExpression &>(expression.GetLeftSide());

        expression.GetRightSide().Accept(*this);
        Value *RightSide = Current;
        if (!RightSide) {
            Current = nullptr;
            return;
        }

        Value *Variable = ValuesByName[LeftSide.GetName()];
        if (!Variable) {
            LogError("Variable unknown");
            Current = nullptr;
            return;
        }

        Builder.CreateStore(RightSide, Variable);
        Current = RightSide;
        return;
    }

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
            break;
    }

    Function *Func = LookupFunction(std::string("binary") + expression.GetOperator());
    if (!Func) {
        LogError("Unknown binary operator");
        Current = nullptr;
        return;
    }

    Value *Args[] = {LeftSide, RightSide};

    Current = Builder.CreateCall(Func, Args, "binop");
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
    Function *Func = Builder.GetInsertBlock()->getParent();
    AllocaInst *Alloca = CreateAlloca(Func, VariableName);

    // emit for:
    expression.GetFor().Accept(*this);
    Value *For = Current;
    if (!For) {
        Current = nullptr;
        return;
    }

    Builder.CreateStore(For, Alloca);

    BasicBlock *LoopBlock = BasicBlock::Create(Context, "loop", Func);

    // fall through from current block to loop block
    Builder.CreateBr(LoopBlock);

    Builder.SetInsertPoint(LoopBlock);

    AllocaInst *OriginalValue = ValuesByName[VariableName];
    ValuesByName[VariableName] = Alloca;

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

    // emit while:
    expression.GetWhile().Accept(*this);
    Value *While = Current;
    if (!While) {
        Current = nullptr;
        return;
    }

    Value *Variable = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca, VariableName.c_str());
    Value *NextVariable = Builder.CreateFAdd(Variable, Step, "nextvar");
    Builder.CreateStore(NextVariable, Alloca);

    // convert to bool (compare non-equal to 0)
    While = Builder.CreateFCmpONE(While, ConstantFP::get(Context, APFloat(0.0)), "loopcond");

    BasicBlock *AfterBlock = BasicBlock::Create(Context, "afterloop", Func);

    Builder.CreateCondBr(While, LoopBlock, AfterBlock);
    Builder.SetInsertPoint(AfterBlock);

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
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(VariableDefinition &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionCall &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionDeclaration &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(FunctionDefinition &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(UnaryExpression &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(BinaryExpression &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(NumExpression &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(ConditionalExpression &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Visit(LoopExpression &expression) {
    IRGenerator->Visit(expression);
    Print();
}

void IRPrinter::Register(std::unique_ptr<FunctionDeclaration> Declaration) {
    IRGenerator->Register(std::move(Declaration));
}

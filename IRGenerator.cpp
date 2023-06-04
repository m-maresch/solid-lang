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

void IRGenerator::Visit(VariableExpression &Expression) {
    AllocaInst *Alloca = ValuesByName[Expression.GetName()];
    if (!Alloca) {
        LogError("Variable unknown");
    }

    Current = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca, Expression.GetName().c_str());
}

void IRGenerator::Visit(VariableDefinition &Expression) {
    std::vector<AllocaInst *> OriginalValues;
    Function *Func = Builder.GetInsertBlock()->getParent();

    for (auto &Variable: Expression.GetVariables()) {
        const auto &VariableName = Variable.first;
        auto VariableInitializer = Variable.second.get();

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

    Expression.GetBody().Accept(*this);
    Value *Body = Current;
    if (!Body) {
        Current = nullptr;
        return;
    }

    unsigned n = Expression.GetVariables().size();
    for (unsigned i = 0; i < n; ++i) {
        ValuesByName[Expression.GetVariables()[i].first] = OriginalValues[i];
    }

    Current = Body;
}

void IRGenerator::Visit(FunctionCall &Expression) {
    Function *Function = LookupFunction(Expression.GetName());
    if (!Function) {
        LogError("Calling unknown function");
        return;
    }

    unsigned n = Expression.GetArguments().size();
    if (Function->arg_size() != n) {
        LogError("Invalid number of arguments passed to function");
        return;
    }

    std::vector<Value *> ArgumentValues;
    for (unsigned i = 0; i < n; ++i) {
        Expression.GetArguments()[i]->Accept(*this);
        ArgumentValues.push_back(Current);
        if (!ArgumentValues.back()) {
            Current = nullptr;
            return;
        }
    }

    Current = Builder.CreateCall(Function, ArgumentValues, "calltmp");
}

void IRGenerator::Visit(FunctionDeclaration &Expression) {
    unsigned n = Expression.GetArguments().size();
    std::vector<Type *> Doubles(n, Type::getDoubleTy(Context));

    FunctionType *FuncType = FunctionType::get(Type::getDoubleTy(Context), Doubles, false);

    Function *Func = Function::Create(FuncType, Function::ExternalLinkage, Expression.GetName(), Module);

    unsigned i = 0;
    for (auto &Argument: Func->args()) {
        Argument.setName(Expression.GetArguments()[i++]);
    }

    Current = Func;
}

void IRGenerator::Visit(FunctionDefinition &Expression) {
    auto Declaration = Expression.TakeDeclaration();
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

    Expression.GetImplementation().Accept(*this);
    if (Value *ReturnValue = Current) {
        Builder.CreateRet(ReturnValue);

        verifyFunction(*Func);

        if (PassManager) {
            PassManager->run(*Func);
        }

        Current = Func;
        return;
    }

    Func->eraseFromParent();
    Current = nullptr;
}

void IRGenerator::Visit(UnaryExpression &Expression) {
    Expression.GetOperand().Accept(*this);
    Value *Operand = Current;
    if (!Operand) {
        Current = nullptr;
        return;
    }

    Function *Func = LookupFunction(std::string("unary") + Expression.GetOperator());
    if (!Func) {
        LogError("Unknown unary operator");
        Current = nullptr;
        return;
    }

    Current = Builder.CreateCall(Func, Operand, "unop");
}

void IRGenerator::Visit(BinaryExpression &Expression) {
    if (Expression.GetOperator() == '=') {
        auto &LeftSide = dynamic_cast<VariableExpression &>(Expression.GetLeftSide());

        Expression.GetRightSide().Accept(*this);
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

    Expression.GetLeftSide().Accept(*this);
    Value *LeftSide = Current;
    Expression.GetRightSide().Accept(*this);
    Value *RightSide = Current;

    if (!LeftSide || !RightSide) {
        Current = nullptr;
        return;
    }

    switch (Expression.GetOperator()) {
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

    Function *Func = LookupFunction(std::string("binary") + Expression.GetOperator());
    if (!Func) {
        LogError("Unknown binary operator");
        Current = nullptr;
        return;
    }

    Value *Args[] = {LeftSide, RightSide};

    Current = Builder.CreateCall(Func, Args, "binop");
}

void IRGenerator::Visit(NumExpression &Expression) {
    Current = ConstantFP::get(Context, APFloat(Expression.GetVal()));
}

void IRGenerator::Visit(ConditionalExpression &Expression) {
    Expression.GetCondition().Accept(*this);
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

    Expression.GetThen().Accept(*this);
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

    Expression.GetOtherwise().Accept(*this);
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

void IRGenerator::Visit(LoopExpression &Expression) {
    std::string VariableName = Expression.GetVariableName();
    Function *Func = Builder.GetInsertBlock()->getParent();
    AllocaInst *Alloca = CreateAlloca(Func, VariableName);

    // emit let:
    Expression.GetLet().Accept(*this);
    Value *Let = Current;
    if (!Let) {
        Current = nullptr;
        return;
    }

    Builder.CreateStore(Let, Alloca);

    BasicBlock *LoopBlock = BasicBlock::Create(Context, "loop", Func);

    // fall through from current block to loop block
    Builder.CreateBr(LoopBlock);

    Builder.SetInsertPoint(LoopBlock);

    AllocaInst *OriginalValue = ValuesByName[VariableName];
    ValuesByName[VariableName] = Alloca;

    // emit loop body:
    Expression.GetBody().Accept(*this);
    if (!Current) {
        Current = nullptr;
        return;
    }

    // emit step:
    Value *Step;
    if (Expression.HasStep()) {
        Expression.GetStep().Accept(*this);
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
    Expression.GetWhile().Accept(*this);
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

void IRPrinter::Visit(VariableExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(VariableDefinition &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(FunctionCall &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(FunctionDeclaration &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(FunctionDefinition &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(UnaryExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(BinaryExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(NumExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(ConditionalExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Visit(LoopExpression &Expression) {
    IRGenerator->Visit(Expression);
    Print();
}

void IRPrinter::Register(std::unique_ptr<FunctionDeclaration> Declaration) {
    IRGenerator->Register(std::move(Declaration));
}

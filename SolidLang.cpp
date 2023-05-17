#include "SolidLang.h"

void SolidLang::Start() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    Lexer = std::make_unique<class Lexer>();
    Parser = std::make_unique<class Parser>(*Lexer);

    fprintf(stderr, "ready> ");
    Lexer->GetNextToken();

    JIT = OnErrorExit(JIT::Create());
    InitLLVM();

    ProcessInput();

    Module->print(errs(), nullptr);
}

void SolidLang::ProcessInput() {
    bool done = false;
    while (!done) {
        fprintf(stderr, "ready> ");
        switch (Lexer->GetCurrentToken()) {
            case t_eof:
                done = true;
                break;
            case ';':
                Lexer->GetNextToken();
                break;
            case t_func:
            case t_operator: {
                std::unique_ptr<Expression> Result = Parser->ParseFunctionDefinition();
                HandleFunction(Result.get());
                break;
            }
            case t_native: {
                std::unique_ptr<FunctionDeclaration> Result = Parser->ParseNative();
                HandleNative(std::move(Result));
                break;
            }
            default: {
                std::unique_ptr<Expression> Result = Parser->ParseTopLevelExpression();
                HandleTopLevelExpression(Result.get());
                break;
            }
        }
    }
}

void SolidLang::InitLLVM() {
    Context = std::make_unique<LLVMContext>();
    Module = std::make_unique<class Module>("Solid JIT", *Context);
    Module->setDataLayout(JIT->getDataLayout());

    PassManager = std::make_unique<legacy::FunctionPassManager>(Module.get());
    PassManager->add(createPromoteMemoryToRegisterPass());
    PassManager->add(createInstructionCombiningPass());
    PassManager->add(createReassociatePass());
    PassManager->add(createGVNPass());
    PassManager->add(createCFGSimplificationPass());
    PassManager->doInitialization();

    Builder = std::make_unique<IRBuilder<>>(*Context);

    IRGenerator = std::make_unique<class IRGenerator>(*Context, *Builder, *Module, *PassManager, ValuesByName,
                                                      FunctionDeclarations);
    IRPrinter = std::make_unique<class IRPrinter>(*IRGenerator);
}

void SolidLang::HandleFunction(Expression *ParsedExpression) {
    if (ParsedExpression) {
        fprintf(stderr, "Successfully parsed\n");
        ParsedExpression->Accept(*IRPrinter);

        OnErrorExit(JIT->addModule(
                ThreadSafeModule(std::move(Module), std::move(Context))
        ));
        InitLLVM();
    } else {
        Lexer->GetNextToken();
    }
}

void SolidLang::HandleNative(std::unique_ptr<FunctionDeclaration> Declaration) {
    if (Declaration) {
        fprintf(stderr, "Successfully parsed\n");
        Declaration->Accept(*IRPrinter);
        IRGenerator->Register(std::move(Declaration));
    } else {
        Lexer->GetNextToken();
    }
}

void SolidLang::HandleTopLevelExpression(Expression *ParsedExpression) {
    if (ParsedExpression) {
        fprintf(stderr, "Successfully parsed\n");
        ParsedExpression->Accept(*IRPrinter);

        auto ResourceTracker = JIT->getMainJITDylib().createResourceTracker();

        OnErrorExit(JIT->addModule(
                ThreadSafeModule(std::move(Module), std::move(Context)),
                ResourceTracker
        ));
        InitLLVM();

        auto TopLevelExprSymbol = OnErrorExit(JIT->lookup("__anonymous_top_level_expr"));
        assert(TopLevelExprSymbol && "Top level expression not found");

        // Cast symbol's address to be able to call it as a native function (no arguments, returns double)
        auto (*TopLevelExpr)() = (double (*)()) (intptr_t) TopLevelExprSymbol.getAddress();
        fprintf(stderr, "Evaluated to %f\n", TopLevelExpr());

        OnErrorExit(ResourceTracker->remove());
    } else {
        Lexer->GetNextToken();
    }
}

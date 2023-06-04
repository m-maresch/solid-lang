#include "SolidLang.h"

int SolidLang::Start() {
    int ExitCode = 0;

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    if (IsRepl()) {
        In = stdin;
    } else {
        In = fopen(InputFile.c_str(), "r");
    }

    Lexer = std::make_unique<class Lexer>(In);
    Parser = std::make_unique<class Parser>(*Lexer);

    IfReplPrint("ready> ");
    Lexer->GetNextToken();

    JIT = OnErrorExit(JIT::Create());
    InitLLVM();

    ProcessInput();

    if (!IsRepl()) {
        fclose(In);
    }

    if (HasOutputFile()) {
        ExitCode = WriteObjectFile();
    }

    if (PrintIR) {
        errs() << "\n";
        Module->print(errs(), nullptr);
    }

    return ExitCode;
}

void SolidLang::ProcessInput() {
    bool done = false;
    while (!done) {
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
                IfReplPrint("ready> ");
                break;
            }
        }
    }
}

void SolidLang::InitLLVM() {
    Context = std::make_unique<LLVMContext>();
    Module = std::make_unique<class Module>("Solid JIT", *Context);
    Module->setDataLayout(JIT->GetDataLayout());

    std::unique_ptr<legacy::FunctionPassManager> PassManager = nullptr;
    if (!IsRepl()) {
        PassManager = std::make_unique<legacy::FunctionPassManager>(Module.get());
        PassManager->add(createPromoteMemoryToRegisterPass());
        PassManager->add(createInstructionCombiningPass());
        PassManager->add(createReassociatePass());
        PassManager->add(createGVNPass());
        PassManager->add(createCFGSimplificationPass());
        PassManager->doInitialization();
    }

    Builder = std::make_unique<IRBuilder<>>(*Context);

    auto IRGenerator = std::make_unique<class IRGenerator>(*Context, *Builder, *Module, std::move(PassManager),
                                                           ValuesByName, FunctionDeclarations);

    if (PrintIR) {
        Visitor = std::make_unique<class IRPrinter>(std::move(IRGenerator));
    } else {
        Visitor = std::move(IRGenerator);
    }
}

int SolidLang::WriteObjectFile() {
    auto TargetTriple = sys::getDefaultTargetTriple();
    Module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    TargetOptions Options;
    auto Machine = Target->createTargetMachine(TargetTriple, CPU, "", Options, std::optional<Reloc::Model>());

    Module->setDataLayout(Machine->createDataLayout());

    std::error_code ErrorCode;
    raw_fd_ostream OutputStream(OutputFile, ErrorCode, sys::fs::OF_None);

    if (ErrorCode) {
        errs() << "could not open file: " << ErrorCode.message();
        return 1;
    }

    legacy::PassManager OutputPassManager;
    if (Machine->addPassesToEmitFile(OutputPassManager, OutputStream, nullptr, CGFT_ObjectFile)) {
        errs() << "could not emit file";
        return 1;
    }

    OutputPassManager.run(*Module);
    OutputStream.flush();

    outs() << "created " << OutputFile << "\n";

    return 0;
}

void SolidLang::HandleFunction(Expression *ParsedExpression) {
    if (ParsedExpression) {
        ParsedExpression->Accept(*Visitor);

        if (IsRepl()) {
            OnErrorExit(JIT->AddModule(
                    ThreadSafeModule(std::move(Module), std::move(Context))
            ));
            InitLLVM();
        }
    } else {
        Lexer->GetNextToken();
    }
}

void SolidLang::HandleNative(std::unique_ptr<FunctionDeclaration> Declaration) {
    if (Declaration) {
        Declaration->Accept(*Visitor);
        Visitor->Register(std::move(Declaration));
    } else {
        Lexer->GetNextToken();
    }
}

void SolidLang::HandleTopLevelExpression(Expression *ParsedExpression) {
    if (ParsedExpression) {
        ParsedExpression->Accept(*Visitor);

        if (IsRepl()) {
            auto ResourceTracker = JIT->GetMain().createResourceTracker();

            OnErrorExit(JIT->AddModule(
                    ThreadSafeModule(std::move(Module), std::move(Context)),
                    ResourceTracker
            ));
            InitLLVM();

            auto TopLevelExprSymbol = OnErrorExit(JIT->Lookup("__anonymous_top_level_expr"));
            assert(TopLevelExprSymbol && "Top level expression not found");

            // Cast symbol's address to be able to call it as a native function (no arguments, returns double)
            auto (*TopLevelExpr)() = (double (*)()) (intptr_t) TopLevelExprSymbol.getAddress();
            fprintf(stderr, "Evaluated to %f\n", TopLevelExpr());

            OnErrorExit(ResourceTracker->remove());
        }
    } else {
        Lexer->GetNextToken();
    }
}

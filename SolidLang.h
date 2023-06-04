#ifndef SOLID_LANG_SOLIDLANG_H
#define SOLID_LANG_SOLIDLANG_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/CommandLine.h"
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include "Lexer.h"
#include "Parser.h"
#include "IRGenerator.h"
#include "JIT.h"
#include "BuiltIns.h"

using namespace llvm;
using namespace llvm::orc;

class SolidLang {

public:
    SolidLang(std::string InputFile, std::string OutputFile, bool PrintIR) : InputFile(std::move(InputFile)),
                                                                             OutputFile(std::move(OutputFile)),
                                                                             PrintIR(PrintIR) {}

    int Start();

private:
    FILE *In;

    std::string InputFile;
    std::string OutputFile;
    bool PrintIR;

    std::unique_ptr<JIT> JIT;
    std::unique_ptr<LLVMContext> Context;
    std::unique_ptr<class Module> Module;

    std::unique_ptr<Lexer> Lexer;
    std::unique_ptr<Parser> Parser;

    std::unique_ptr<IRBuilder<>> Builder;
    std::unique_ptr<ExpressionVisitor> Visitor;

    std::map<std::string, AllocaInst *> ValuesByName;
    std::map<std::string, std::unique_ptr<FunctionDeclaration>> FunctionDeclarations;

    ExitOnError OnErrorExit;

    void ProcessInput();

    void InitLLVM();

    int WriteObjectFile();

    void HandleFunction(Expression *ParsedExpression);

    void HandleNative(std::unique_ptr<FunctionDeclaration> Declaration);

    void HandleTopLevelExpression(Expression *ParsedExpression);

    bool IsRepl() {
        return InputFile == "-";
    }

    bool HasOutputFile() {
        return OutputFile != "-";
    }

    void IfReplPrint(const char *Message) {
        if (IsRepl()) {
            fprintf(stderr, "%s", Message);
        }
    }
};

#endif

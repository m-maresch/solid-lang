#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "Lexer.h"
#include "Parser.h"
#include "IRGenerator.h"
#include "JIT.h"

using namespace llvm;
using namespace llvm::orc;

void HandleExpression(Lexer &Lexer, Expression *ParsedExpression, Visitor &Visitor) {
    if (ParsedExpression) {
        fprintf(stderr, "Successfully parsed\n");
        ParsedExpression->Accept(Visitor);
    } else {
        Lexer.GetNextToken();
    }
}

int main() {
    auto Lexer = std::make_unique<class Lexer>();
    auto Parser = std::make_unique<class Parser>(*Lexer);

    fprintf(stderr, "ready> ");
    Lexer->GetNextToken();

    ExitOnError OnErrorExit;
    std::unique_ptr<JIT> JIT = OnErrorExit(JIT::Create());

    std::unique_ptr<LLVMContext> Context = std::make_unique<LLVMContext>();
    std::unique_ptr<Module> Module = std::make_unique<class Module>("Solid JIT", *Context);
    std::unique_ptr<IRBuilder<>> Builder = std::make_unique<IRBuilder<>>(*Context);
    std::unique_ptr<IRGenerator> IRGenerator = std::make_unique<class IRGenerator>(*Context, *Builder, *Module);
    std::unique_ptr<IRPrinter> IRPrinter = std::make_unique<class IRPrinter>(*IRGenerator);

    bool done = false;
    while (!done) {
        fprintf(stderr, "ready> ");
        switch (Lexer->GetCurrentToken()) {
            case token_eof:
                done = true;
                break;
            case ';':
                Lexer->GetNextToken();
                break;
            case token_func: {
                std::unique_ptr<Expression> Result = Parser->ParseFunctionDefinition();
                HandleExpression(*Lexer, Result.get(), *IRPrinter);
                break;
            }
            case token_extern: {
                std::unique_ptr<Expression> Result = Parser->ParseExtern();
                HandleExpression(*Lexer, Result.get(), *IRPrinter);
                break;
            }
            default: {
                std::unique_ptr<Expression> Result = Parser->ParseTopLevelExpression();
                HandleExpression(*Lexer, Result.get(), *IRPrinter);
                break;
            }
        }
    }

    Module->print(errs(), nullptr);
    return 0;
}

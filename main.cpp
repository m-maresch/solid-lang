#include <iostream>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "Lexer.h"
#include "Parser.h"


void HandleExpression(const std::shared_ptr<class Lexer> &Lexer, std::unique_ptr<Expression> ParsedExpression) {
    if (ParsedExpression) {
        fprintf(stderr, "Successfully parsed\n");
    } else {
        Lexer->GetNextToken();
    }
}

int main() {
    auto Lexer = std::make_shared<class Lexer>();
    auto Parser = std::make_unique<class Parser>(Lexer);

    fprintf(stderr, "ready> ");
    Lexer->GetNextToken();

    while (true) {
        fprintf(stderr, "ready> ");
        switch (Lexer->GetCurrentToken()) {
            case token_eof:
                return 0;
            case ';':
                Lexer->GetNextToken();
                break;
            case token_func: {
                std::unique_ptr<Expression> Result = Parser->ParseFunctionDefinition();
                HandleExpression(Lexer, std::move(Result));
                break;
            }
            case token_extern: {
                std::unique_ptr<Expression> Result = Parser->ParseExtern();
                HandleExpression(Lexer, std::move(Result));
                break;
            }
            default: {
                std::unique_ptr<Expression> Result = Parser->ParseTopLevelExpression();
                HandleExpression(Lexer, std::move(Result));
                break;
            }
        }
    }
}

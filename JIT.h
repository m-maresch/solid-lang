// Based on: https://llvm.org/docs/tutorial/BuildingAJIT2.html

#ifndef SOLID_LANG_JIT_CPP
#define SOLID_LANG_JIT_CPP

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include <llvm/Transforms/Utils.h>
#include <memory>

using namespace llvm;
using namespace llvm::orc;

class JIT {
private:
    std::unique_ptr<ExecutionSession> ES;

    DataLayout DL;
    MangleAndInterner Mangle;

    RTDyldObjectLinkingLayer ObjectLayer;
    IRCompileLayer CompileLayer;
    IRTransformLayer OptimizeLayer;

    JITDylib &Main;

public:
    JIT(std::unique_ptr<ExecutionSession> ES, JITTargetMachineBuilder JTMB, const DataLayout &DL)
            : ES(std::move(ES)), DL(DL), Mangle(*this->ES, this->DL),
              ObjectLayer(
                      *this->ES,
                      []() { return std::make_unique<SectionMemoryManager>(); }
              ),
              CompileLayer(
                      *this->ES,
                      ObjectLayer,
                      std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))
              ),
              OptimizeLayer(
                      *this->ES,
                      CompileLayer,
                      OptimizeModule
              ),
              Main(this->ES->createBareJITDylib("<main>")) {
        Main.addGenerator(cantFail(
                DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())
        ));
        if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
            ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    ~JIT() {
        if (auto Err = ES->endSession())
            ES->reportError(std::move(Err));
    }

    static Expected<std::unique_ptr<JIT>> Create() {
        auto EPC = SelfExecutorProcessControl::Create();
        if (!EPC)
            return EPC.takeError();

        auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

        JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

        auto DL = JTMB.getDefaultDataLayoutForTarget();
        if (!DL)
            return DL.takeError();

        return std::make_unique<JIT>(std::move(ES), std::move(JTMB), std::move(*DL));
    }

    const DataLayout &GetDataLayout() const { return DL; }

    JITDylib &GetMain() { return Main; }

    Error AddModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
        if (!RT)
            RT = Main.getDefaultResourceTracker();
        return OptimizeLayer.add(RT, std::move(TSM));
    }

    Expected<JITEvaluatedSymbol> Lookup(StringRef Name) {
        return ES->lookup({&Main}, Mangle(Name.str()));
    }

private:
    static Expected<ThreadSafeModule> OptimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &MR) {
        TSM.withModuleDo([](Module &Mod) {
            auto PassManager = std::make_unique<legacy::FunctionPassManager>(&Mod);

            PassManager->add(createPromoteMemoryToRegisterPass());
            PassManager->add(createInstructionCombiningPass());
            PassManager->add(createReassociatePass());
            PassManager->add(createGVNPass());
            PassManager->add(createCFGSimplificationPass());
            PassManager->doInitialization();

            for (auto &Func: Mod)
                PassManager->run(Func);
        });

        return std::move(TSM);
    }
};

#endif

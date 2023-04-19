// Based on: https://github.com/llvm/llvm-project/blob/main/llvm/examples/Kaleidoscope/include/KaleidoscopeJIT.h

#ifndef SOLID_LANG_JIT_CPP
#define SOLID_LANG_JIT_CPP

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
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

    JITDylib &MainJD;

public:
    JIT(std::unique_ptr<ExecutionSession> ES, JITTargetMachineBuilder JTMB, DataLayout DL)
            : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
              ObjectLayer(*this->ES,
                          []() { return std::make_unique<SectionMemoryManager>(); }),
              CompileLayer(*this->ES, ObjectLayer,
                           std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
              MainJD(this->ES->createBareJITDylib("<main>")) {
        MainJD.addGenerator(
                cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
                        DL.getGlobalPrefix())));
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

        JITTargetMachineBuilder JTMB(
                ES->getExecutorProcessControl().getTargetTriple());

        auto DL = JTMB.getDefaultDataLayoutForTarget();
        if (!DL)
            return DL.takeError();

        return std::make_unique<JIT>(std::move(ES), std::move(JTMB),
                                     std::move(*DL));
    }

    const DataLayout &getDataLayout() const { return DL; }

    JITDylib &getMainJITDylib() { return MainJD; }

    Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
        if (!RT)
            RT = MainJD.getDefaultResourceTracker();
        return CompileLayer.add(RT, std::move(TSM));
    }

    Expected<JITEvaluatedSymbol> lookup(StringRef Name) {
        return ES->lookup({&MainJD}, Mangle(Name.str()));
    }
};

#endif

#include "SolidLang.h"

cl::OptionCategory Compiler("Compiler options");
cl::opt<std::string> InputFile(cl::Positional, cl::desc("<input filename>"), cl::init("-"), cl::cat(Compiler));
cl::opt<std::string> OutputFile("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::init("-"),
                                cl::cat(Compiler));
cl::opt<bool> PrintIR("IR", cl::desc("Print generated LLVM IR"));

int main(int argc, char **argv) {
    cl::HideUnrelatedOptions(Compiler);
    cl::ParseCommandLineOptions(argc, argv, "The Solid Programming Language");

    if (InputFile != "-" && OutputFile == "-") {
        OutputFile = InputFile.substr(0, InputFile.find_last_of("."));
    }

    if (OutputFile != "-" && OutputFile.substr(OutputFile.size() - 2) != std::string(".o")) {
        OutputFile += ".o";
    }

    auto SolidLang = std::make_unique<class SolidLang>(InputFile, OutputFile, PrintIR);
    return SolidLang->Start();
}

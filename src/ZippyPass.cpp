#include "ZippyCommon.hpp"
#include "FunctionInfo.hpp"
#include "FieldInfo.hpp"
#include "StructInfo.hpp"

#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>


namespace Zippy {
    class Pass {
        llvm::Module &M;
        llvm::ModuleAnalysisManager &AM;

        // Using lists instead of vectors, because using vectors didn't let me remove elements?
        std::vector<StructInfo> structInfos;
        std::vector<FunctionInfo> functionInfos;

        bool collectStructTypes() {
            structInfos = StructInfo::collect(M);
            return !structInfos.empty();
        }

        bool collectFunctions() {
            functionInfos = FunctionInfo::collect(M);
            return !functionInfos.empty();
        }

        bool collectFieldUses() {
            llvm::errs() << "Collecting Field Uses\n";
            unsigned sumUses = 0;
            for (auto &structInfo: structInfos) {
                llvm::errs() << TAB_STR << structInfo.getStructType() << "\n";
                for (auto &functionInfo: functionInfos) {
                    const auto uses = structInfo.collectFieldUses(functionInfo);
                    if (uses == 0) continue;
                    llvm::errs() << TAB_STR << TAB_STR << functionInfo.getFunction();
                    llvm::errs() << llvm::format(" [%d] uses\n", uses);
                    sumUses += uses;
                }
            }
            if (sumUses == 0) {
                llvm::errs() << "No Field Uses collected\n";
                return false;
            }
            llvm::errs() << llvm::format("Collected [%d] Field Uses\n", sumUses);
            return true;
        }

    public:
        explicit Pass(llvm::Module &M,
                      llvm::ModuleAnalysisManager &AM): M(M), AM(AM) {}

        llvm::PreservedAnalyses run() {
            for (auto &globalVarRaw: M.globals()) {
                const GlobalVariable globalVar = {&globalVarRaw};
                if (globalVar.isNonZeroInit()) {
                    llvm::errs() << globalVar << "\n";
                }
            }

            auto didWork = false;

            if (!collectStructTypes()) goto no_work;
            if (!collectFunctions()) goto no_work;
            if (!collectFieldUses()) goto no_work;
            llvm::errs() << "Getting to work\n";

            for (auto &structInfo: structInfos) {
                llvm::errs() << "Processing Struct: ";
                structInfo.getStructType().printName(llvm::errs());
                llvm::errs() << " \n";

                // Naive sort by number-of-uses
                std::ranges::sort(structInfo.getFieldInfos(), std::greater(), &FieldInfo::getTargetIndex);

                structInfo.updateTargetIndices();
                didWork |= structInfo.applyRemap();
            }

            if (didWork) {
                llvm::errs() << "Did work\n";
                return llvm::PreservedAnalyses::none();
            }

            return llvm::PreservedAnalyses::all();
            // No work skip
        no_work:
            llvm::errs() << "No work found\n";
            return llvm::PreservedAnalyses::all();
        }
    };

    struct ZippyPass : llvm::PassInfoMixin<ZippyPass> {
        static llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
            return Pass(M, AM).run();
        }
    };
}

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "ZippyPass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    // Allows this plugin to run when using opt with the pass named 'zippy'
                    //
                    // eg: -load-pass-plugin ZippyPass.so -passes=zippy input.ll -o output.ll -S
                    if (Name == "zippy") {
                        // This is your pass's pipeline name
                        MPM.addPass(Zippy::ZippyPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

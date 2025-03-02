#include "ModuleContext.hpp"

#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

namespace Zippy {
    class Pass {
        llvm::Module &M;
        llvm::ModuleAnalysisManager &AM;

        std::vector<StructInfo> structInfos;
        std::vector<FunctionInfo> functionInfos;

        bool init() {
            if (!collectStructTypes()) return false;
            if (!collectFunctions()) return false;
            if (!collectFieldUsages()) return false;
            return true;
        }

        bool collectStructTypes() {
            llvm::errs() << "Collecting Structs\n";
            for (const auto structType: M.getIdentifiedStructTypes()) {
                auto structInfo = structInfos.emplace_back(StructType{structType});
                llvm::errs() << TAB_STR << structInfo.getStructType() << "\n";
            }
            if (structInfos.empty()) {
                llvm::errs() << "No Structs collected\n";
                return false;
            }
            llvm::errs() << llvm::format("Collected [%d] Structs\n", structInfos.size());
            return true;
        }

        bool collectFunctions() {
            llvm::errs() << "Collecting Function\n";
            for (auto &functionRaw: M.functions()) {
                Function function{&functionRaw};
                llvm::errs() << TAB_STR << function;
                if (!function.isDefined()) {
                    llvm::errs() << " - Not defined, skipped\n";
                    continue;
                }
                FunctionInfo functionInfo(function);
                const auto gepRefs = functionInfo.getGepRefs();
                if (gepRefs.empty()) {
                    llvm::errs() << " - No struct references, skipped\n";
                    continue;
                }
                functionInfos.push_back(functionInfo);
                llvm::errs() << llvm::format(" - Found [%d] GEPs\n", gepRefs.size());
            }
            if (functionInfos.empty()) {
                llvm::errs() << "No Functions collected\n";
                return false;
            }
            llvm::errs() << llvm::format("Collected [%d] Functions\n", functionInfos.size());
            return true;
        }

        bool collectFieldUsages() {
            llvm::errs() << "Collecting field usages...\n";
            unsigned allUsages = 0;
            for (auto structInfo: structInfos) {
                for (const auto functionInfo: functionInfos) {
                    structInfo.collectFieldUsages(functionInfo);
                }
                allUsages += structInfo.getSumFieldUsages();
            }
            if (allUsages == 0) {
                llvm::errs() << "No field usages collected\n";
                return false;
            }
            llvm::errs() << llvm::format("Collected [%d] field usages\n", allUsages);
            return true;
        }

    public:
        explicit Pass(llvm::Module &M,
                      llvm::ModuleAnalysisManager &AM): M(M), AM(AM) {}

        llvm::PreservedAnalyses run() {
            if (!init()) {
                llvm::errs() << "No work found\n";
                return llvm::PreservedAnalyses::all();
            }
            llvm::errs() << "Getting to work\n";

            return llvm::PreservedAnalyses::none();
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

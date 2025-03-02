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
        std::list<StructInfo> structInfos;
        std::list<FunctionInfo> functionInfos;

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

        bool skipUnused() {
            // Prune unused structs
            for (auto it = structInfos.begin(); it != structInfos.end();) {
                if (const auto structInfo = *it; structInfo.getSumFieldUses() == 0) {
                    llvm::errs() << "Skipping Struct " << structInfo.getStructType() << " no usages found\n";
                    it = structInfos.erase(it);
                } else {
                    ++it;
                }
            }
            // Prune unused Functions
            for (auto it = functionInfos.begin(); it != functionInfos.end();) {
                if (const auto functionInfo = *it; functionInfo.getNumUsedGepRefs() == 0) {
                    llvm::errs() << "Skipping Function " << functionInfo.getFunction() << " no usages found\n";
                    it = functionInfos.erase(it);
                } else {
                    ++it;
                }
            }
            // Makes sure we still have stuff to work with, otherwise exit
            auto hasWork = true;
            if (structInfos.empty()) {
                llvm::errs() << "All Structs Skipped\n";
                hasWork = false;
            }
            if (functionInfos.empty()) {
                llvm::errs() << "All Functions Skipped\n";
                hasWork = false;
            }
            return hasWork;
        }

    public:
        explicit Pass(llvm::Module &M,
                      llvm::ModuleAnalysisManager &AM): M(M), AM(AM) {}

        llvm::PreservedAnalyses run() {
            if (!collectStructTypes()) goto no_work;
            if (!collectFunctions()) goto no_work;
            if (!collectFieldUses()) goto no_work;
            if (!skipUnused()) goto no_work;

            llvm::errs() << "Getting to work\n";
            return llvm::PreservedAnalyses::none();

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

#include "ZippyCommon.hpp"
#include "FunctionInfo.hpp"
#include "FieldInfo.hpp"
#include "GlobalVarInfo.hpp"
#include "StructInfo.hpp"

#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>


namespace Zippy {
    class Pass {
        llvm::Module &M;
        llvm::ModuleAnalysisManager &AM;
        const llvm::DataLayout &DL;

        // Using lists instead of vectors, because using vectors didn't let me remove elements?
        std::vector<StructInfo> structInfos;
        std::vector<FunctionInfo> functionInfos;

        bool collectStructTypes() {
            structInfos = StructInfo::collect(M, DL);
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

        void collectGlobalVars() {
            auto globalVarInfos = GlobalVarInfo::collect(M);
            if (globalVarInfos.empty()) return;
            unsigned varsCollected = 0;
            llvm::errs() << "Collecting Global Variables into Structs\n";
            for (auto &structInfo: structInfos) {
                varsCollected += structInfo.collectGlobalVars(globalVarInfos);
            }
            if (varsCollected == 0) {
                llvm::errs() << "None collected\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Total Global Variables\n", varsCollected);
            }
        }

        // Inner loop multipliers, scaled by a non-linear curve as to not give them too much weight
        const float outerLoopMult = std::pow(100, 1.3F);
        const float middleLoopMult = std::pow(20, 1.3F);
        const float innerLoopMult = std::pow(10, 1.3F);

        void computeFieldWeights() {
            for (auto &structInfo: structInfos) {
                structInfo.computeFieldWeights([*this](FieldInfo &fieldInfo) {
                    const auto originalIndex = fieldInfo.getOriginalIndex();
                    auto &uses = fieldInfo.getUses();

                    float loopAccessWeight = 1.0F;
                    // No uses means default weight
                    if (uses.empty()) return;

                    // Debug counters
                    unsigned loopAccessCount = 0;
                    unsigned deepestLoopFound = 0;

                    for (const auto &use: uses) {
                        const unsigned depth = use.computeLoopDepth();
                        // No work to do if depth is zero
                        if (depth == 0) continue;

                        // TODO: In concept, values present in an outer loop would run more frequently than
                        //       inside an inner loop. Consider iterating over x-y-z, but doing intermediate work
                        //       at each stage. As far as frequency of access goes this is weird, might change it as
                        //       benchmarks and tests roll along.
                        float loopMult;
                        switch (depth) {
                            case 1:
                                loopMult = outerLoopMult;
                                break;
                            case 2:
                                loopMult = middleLoopMult;
                                break;
                            default:
                                loopMult = innerLoopMult;
                                break;
                        }

                        // Compute new use weight and apply iy
                        const float useWeight = loopMult * depth;
                        loopAccessWeight = std::max(loopAccessWeight, useWeight);

                        // Increment debug counters
                        ++loopAccessCount;
                        deepestLoopFound = std::max(deepestLoopFound, depth);
                    }

                    // Print debug counters
                    if (loopAccessCount > 0) {
                        llvm::errs() << llvm::format("Field %d accesses in loops: %d, deepest loop: %d, weight: %d\n",
                                                     originalIndex, loopAccessCount, deepestLoopFound,
                                                     loopAccessWeight);
                    } else {
                        llvm::errs() << llvm::format("Field %d not used in loops. ", originalIndex);
                    }

                    // We set the loop depth for future debug prints
                    fieldInfo.setLoopAccessWeight(loopAccessWeight);
                    fieldInfo.setTotalWeight(static_cast<float>(fieldInfo.getSumLoadStores()) * loopAccessWeight);
                });
            }
        }

    public:
        explicit Pass(llvm::Module &M,
                      llvm::ModuleAnalysisManager &AM): M(M), AM(AM), DL(M.getDataLayout()) {
        }

        llvm::PreservedAnalyses run() {
            auto didWork = false;

            if (!collectStructTypes()) goto no_work;
            if (!collectFunctions()) goto no_work;
            if (!collectFieldUses()) goto no_work;
            collectGlobalVars();
            computeFieldWeights();

            llvm::errs() << "Getting to work\n";

            for (auto &structInfo: structInfos) {
                llvm::errs() << "Processing Struct: ";
                structInfo.getStructType().printName(llvm::errs());
                llvm::errs() << "\n";

                llvm::errs() << TAB_STR << "Original order as-found:\n";
                for (auto &fieldInfo: structInfo.getFieldInfos()) {
                    llvm::errs() << llvm::format("Field %d: loads=%d, stores=%d, loop_weight=%f, total_weight=%f ",
                                                 fieldInfo.getOriginalIndex(), fieldInfo.getNumLoads(),
                                                 fieldInfo.getNumStores(), fieldInfo.getLoopAccessWeight(),
                                                 fieldInfo.getTotalWeight());
                }

                // Sort using both access count and loop weight
                std::ranges::sort(structInfo.getFieldInfos(), std::greater(), &FieldInfo::getTotalWeight);

                // TODO: Print debug info
                structInfo.updateTargetIndices();
                if (structInfo.applyRemap()) {
                    didWork = true;
                } else {
                    continue;
                }
                structInfo.applyAlign(DL);
                structInfo.updateMemCpyRefs(M.getDataLayout());
            }

            if (didWork) {
                llvm::errs() << "Did work\n";
                return llvm::PreservedAnalyses::none();
            }
            llvm::errs() << "Did no work\n";
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
                [](const StringRef Name, ModulePassManager &MPM,
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

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
                    llvm::errs() << TAB_STR_2 << functionInfo.getFunction();
                    llvm::errs() << llvm::format(" [%d] uses\n", uses);
                    sumUses += uses;
                }
            }
            if (sumUses == 0) {
                llvm::errs() << "No Field Uses collected\n";
                return false;
            }
            llvm::errs() << llvm::format("Collected [%d] Field Uses\n\n", sumUses);
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
                llvm::errs() << "Computing Field Weights For: " << structInfo.getStructType() << "\n";
                auto &fieldInfos = structInfo.getFieldInfos();

                llvm::errs() << TAB_STR << "Size Weights:\n";
                for (auto &fieldInfo: fieldInfos) {
                    const auto size = fieldInfo.getAllocSize().getKnownMinValue();
                    const float sizeWeight = size;
                    fieldInfo.setSizeWeight(sizeWeight);
                    llvm::errs() << TAB_STR_2 << llvm::format(
                        "Index: [%02d] - Alloc Size: [%02d] - Size Weight: [%06.2f]\n",
                        fieldInfo.getInitialIndex(), size, sizeWeight);
                }

                llvm::errs() << TAB_STR << "Load/Store Weights:\n";
                for (auto &fieldInfo: fieldInfos) {
                    llvm::errs() << TAB_STR_2 << llvm::format("Index: [%02d] - ", fieldInfo.getInitialIndex());
                    if (fieldInfo.getSumLoadStores() == 0) {
                        fieldInfo.setLoadWeight(0.0F);
                        fieldInfo.setStoreWeight(0.0F);
                        llvm::errs() << "No Load/Stores\n";
                        continue;
                    }

                    const auto loads = fieldInfo.getNumLoads();
                    const auto stores = fieldInfo.getNumStores();

                    const float loadWeight = loads;
                    const float storeWeight = stores;

                    fieldInfo.setLoadWeight(loadWeight);
                    fieldInfo.setStoreWeight(storeWeight);

                    llvm::errs() << llvm::format(
                        "Loads: [%02d] - Load Weight: [%06.2f] - Stores: [%02d] - Store Weight: [%06.2f]\n",
                        loads, loadWeight, stores, storeWeight);
                }

                llvm::errs() << TAB_STR << "Loop Weights:\n";
                for (auto &fieldInfo: fieldInfos) {
                    float loopAccessWeight = 1.0F;
                    unsigned loopAccessCount = 0;
                    unsigned deepestLoopFound = 0;

                    const auto &uses = fieldInfo.getUses();
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

                    llvm::errs() << TAB_STR_2;
                    if (loopAccessCount > 0) {
                        llvm::errs() << llvm::format(
                            "Index: [%02d] - Loop Accesses: [%02d] - Deepest Loop: [%02d] - Loop Weight: [%06.2f]\n",
                            fieldInfo.getInitialIndex(), loopAccessCount, deepestLoopFound, loopAccessWeight);
                    } else {
                        llvm::errs() << llvm::format("Index: [%02d] - Not used in loops.\n",
                                                     fieldInfo.getInitialIndex());
                    }

                    fieldInfo.setLoopWeight(loopAccessWeight);
                }

                structInfo.normalizeWeights();

                auto maxSizeWeight = 1.0F;
                auto maxLoadWeight = 1.0F;
                auto maxStoreWeight = 1.0F;
                auto maxLoopWeight = 1.0F;

                llvm::errs() << TAB_STR << "Normalized Weights:\n";

                // Find maximum weights
                for (const auto &fieldInfo: fieldInfos) {
                    maxSizeWeight = std::max(maxSizeWeight, fieldInfo.getSizeWeight());
                    maxLoadWeight = std::max(maxLoadWeight, fieldInfo.getLoadWeight());
                    maxStoreWeight = std::max(maxStoreWeight, fieldInfo.getStoreWeight());
                    maxLoopWeight = std::max(maxLoopWeight, fieldInfo.getLoopWeight());
                }
                // Normalized weights (0, 1)
                for (auto &fieldInfo: fieldInfos) {
                    const auto sizeWeight = fieldInfo.getSizeWeight() / maxSizeWeight;
                    const auto loadWeight = fieldInfo.getLoadWeight() / maxLoadWeight;
                    const auto storeWeight = fieldInfo.getStoreWeight() / maxStoreWeight;
                    const auto loopWeight = fieldInfo.getLoopWeight() / maxLoopWeight;

                    fieldInfo.setSizeWeight(sizeWeight);
                    fieldInfo.setLoadWeight(loadWeight);
                    fieldInfo.setStoreWeight(storeWeight);
                    fieldInfo.setLoopWeight(loopWeight);

                    llvm::errs() << TAB_STR_2 << llvm::format(
                        "Index: [%02d] - Size Weight: [%06.2f] - Load Weight: [%06.2f] - Store Weight: [%06.2f] - Loop Weight: [%06.2f]\n",
                        sizeWeight, loadWeight, storeWeight, loopWeight);
                }

                llvm::errs() << TAB_STR << "Total Weights:\n";
                for (auto &fieldInfo: fieldInfos) {
                    auto totalWeight = 0.0F;

                    if (fieldInfo.getSumLoadStores() != 0) {
                        // For used fields, prioritize:
                        // 1. Fields accessed in loops (highest priority)
                        // 2. Fields frequently accessed (load/store frequency)
                        // 3. With a slight bias toward larger fields to reduce padding

                        totalWeight = fieldInfo.getLoopWeight() * 0.6F + // Loop access is most important
                                      fieldInfo.getLoadWeight() * 0.3F + // Read operations
                                      fieldInfo.getStoreWeight() * 0.2F + // Write operations
                                      fieldInfo.getSizeWeight() * 0.1F; // Small bias for larger fields
                    } else {
                        // For unused fields, still give a slight preference to larger types
                        // to help reduce padding when grouped together
                        totalWeight = 0.1F + fieldInfo.getSizeWeight() * 0.05F;
                    }

                    fieldInfo.setTotalWeight(totalWeight);
                    llvm::errs() << TAB_STR_2 << llvm::format("Index: [%02d] - Total Weight: [%06.2f]\n",
                                                              fieldInfo.getCurrentIndex(), totalWeight);
                }
            }
            llvm::errs() << "\n";
        }

    public:
        explicit Pass(llvm::Module &M,
                      llvm::ModuleAnalysisManager &AM): M(M), AM(AM), DL(M.getDataLayout()) {}

        llvm::PreservedAnalyses run() {
            auto didWork = false;

            if (!collectStructTypes()) goto no_work;
            if (!collectFunctions()) goto no_work;
            if (!collectFieldUses()) goto no_work;
            collectGlobalVars();
            computeFieldWeights();

            for (auto &structInfo: structInfos) {
                llvm::errs() << "Transforming: " << structInfo.getStructType() << "\n";

                auto &fieldInfos = structInfo.getFieldInfos();
                std::sort(fieldInfos.begin(), fieldInfos.end(),
                          [](const FieldInfo &a, const FieldInfo &b) {
                              return a.getTotalWeight() > b.getTotalWeight();
                          });

                didWork |= structInfo.applyTransform(DL);
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

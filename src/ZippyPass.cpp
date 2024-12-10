#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
#pragma region LLVM Print Utils
// List all structs in the module
void listStructs(Module &M) {
    errs() << "Found Structures:\n";
    for (auto &ST: M.getIdentifiedStructTypes()) {
        errs() << "  Struct: " << ST->getName() << "\n";
        if (!ST->isOpaque()) {
            errs() << "    Elements (" << ST->getNumElements() << "):\n";
            for (unsigned i = 0; i < ST->getNumElements(); i++) {
                errs() << "      [" << i << "] "
                        << *ST->getElementType(i) << "\n";
            }
        }
        errs() << "\n";
    }
}

// List all functions in the module
void listFunctions(Module &M) {
    errs() << "Found Functions:\n";
    for (auto &F: M) {
        errs() << "  Function: " << F.getName() << "\n";
        errs() << "    Return Type: " << *F.getReturnType() << "\n";
        errs() << "    Arguments (" << F.arg_size() << "):\n";
        for (auto &Arg: F.args()) {
            errs() << "      " << *Arg.getType() << " "
                    << Arg.getName() << "\n";
        }
        errs() << "    Basic Blocks: " << F.size() << "\n\n";
    }
}
#pragma endregion

typedef uint64_t u64;

template<typename V>
using Vector = std::vector<V>;

template<typename K, typename V>
using Map = std::unordered_map<K, V>;

struct StructMetadata {
    StructType *target;

    Map<u64, u64> layoutRemap;
    Vector<Vector<GetElementPtrInst *>> funcGEPList;

    explicit StructMetadata(StructType *target) : target(target) {
        for (auto i = 0; i < target->getNumElements(); i++)
            layoutRemap[i] = i;
        errs() << "Created struct metadata for: ";
        target->print(errs(), true);
        errs() << "\n";
    }

    bool addElemUse(Function &F, u64 funcNum, GetElementPtrInst *GEP) {
        if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(2))) {
            auto isNewFunc = funcGEPList.size() <= funcNum;
            if (isNewFunc) {
                errs() <<
                        "Started tracking elements of Struct: " <<
                        (target->hasName() ? target->getName() : "[UNNAMED]") <<
                        " on Function: " <<
                        (F.hasName() ? F.getName() : "[UNNAMED]") <<
                        "\n";
                funcGEPList.resize(funcNum + 1);
            }
            funcGEPList[funcNum].push_back(GEP);

            return isNewFunc;
        }
        // Should never happen, but might?
        report_fatal_error("Operand 2 of GEP not a ConstantInt");
    }
};

struct ZippyPass : PassInfoMixin<ZippyPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        Map<StructType *, StructMetadata> structMap;

        auto funcNum = 0;
        for (auto &F: M) {
            // Skips `declare`, we only want `define`
            if (F.isDeclaration())
                continue;

            errs() << "Entering function: " << (F.hasName() ? F.getName() : "[UNNAMED]") << "\n";
            for (auto &BB: F) {
                for (auto &I: BB) {
                    if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                        if (auto *ST = dyn_cast<StructType>(GEP->getSourceElementType())) {
                            errs() << "Found Struct GEP: ";
                            GEP->print(errs(), true);
                            auto gepOperandCount = GEP->getNumOperands();

                            // Generally if we have 3 operand, the indexing is into a stride
                            // and then into a field of a struct.
                            //
                            // 4 is also valid if the field is a struct, where the last operand is
                            // an index into the array referenced inside the field.
                            if (gepOperandCount < 3) {
                                errs() << " (Unexpected operand count, skipping)\n";
                                continue;
                            }
                            errs() << "\n";

                            auto &metadata = structMap.try_emplace(ST, ST).first->second;
                            metadata.addElemUse(F, funcNum, GEP);
                        }
                    }
                }
            }
            errs() << "\n";
            funcNum++;
        }

        for (auto &[ST, structMeta]: structMap) {
            auto target = structMeta.target;
            errs() << "STRUCT: " << (target->hasName() ? target->getName() : "[UNNAMED]")  << "\n";
            for (auto &z: structMeta.funcGEPList) {
                errs() << "_S_\n";
                for (auto GEP: z) {
                    if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(2))) {
                        errs() << CI->getZExtValue() << "\n";
                    }
                }
                errs() << "_E_\n";
            }

            // errs() << target->getName() << " " << CI->getZExtValue() << "\n";

            // Create the new constant value you want
            // auto *newIndex = ConstantInt::get(CI->getIntegerType(), 1L);
            // GEP->setOperand(GEP->getNumOperands() - 1, newIndex);
            // return PreservedAnalyses::none();
        }

        return PreservedAnalyses::none();
    }
};
}

#pragma region LLVM Plugin

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
                        MPM.addPass(ZippyPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

#pragma endregion

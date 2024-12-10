#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"

namespace {
const std::string TAB_STR = "    ";

class ZippyPassImpl {
    llvm::Module *module;
    bool didWork = false;

    std::vector<llvm::StructType *> structTypes;
    std::vector<llvm::Function *> functions;

    llvm::DenseMap<llvm::StructType *, std::vector<llvm::GetElementPtrInst *>> fieldUsages;

public:
    explicit ZippyPassImpl(llvm::Module *module)
        : module(module) {
    }

    bool tryRun() {
        enterModule();

        if (!(findStructTypes() && findFunctions() && findFieldUses())) {
            llvm::errs() << "No work in module\n";
            return exitModule();
        }

        return exitModule();
    }

private:
    bool findStructTypes() {
        structTypes = module->getIdentifiedStructTypes();
        if (structTypes.empty()) {
            llvm::errs() << "No struct types found\n";
            return false;
        } else {
            llvm::errs() << "Found (" << structTypes.size() << ") struct types:\n";

            for (auto i = 0; i < structTypes.size(); i++) {
                const auto structType = structTypes[i];
                llvm::errs() << TAB_STR << "ST[" << i << "] -> ";
                structType->print(llvm::errs(), true);
                llvm::errs() << "\n";
            }
            return true;
        }
    }

    bool findFunctions() {
        // Skips declarations, eg functions defined elsewhere
        for (auto &function: module->functions())
            if (!function.isDeclaration())
                functions.push_back(&function);

        if (functions.empty()) {
            llvm::errs() << "No functions found\n";
            return false;
        } else {
            llvm::errs() << "Found (" << functions.size() << ") functions:\n";

            for (auto i = 0; i < functions.size(); i++) {
                const auto function = functions[i];
                llvm::errs() << TAB_STR << "F[" << i << "] -> ";
                llvm::errs() << (function->hasName() ? function->getName() : "?") << " = ";
                function->getFunctionType()->print(llvm::errs(), true);
                llvm::errs() << "\n";
            }
            return true;
        }
    }

    bool findFieldUses() {
        for (auto structType: structTypes) {
            fieldUsages.FindAndConstruct(structType);
        }

        auto foundUsesInModule = false;
        for (auto i = 0; i < functions.size(); i++) {
            const auto function = functions[i];
            auto foundUsesInFunction = false;
            llvm::errs() << "Checking: F[" << i << "] for field usage\n";

            for (auto &inst: function->getEntryBlock()) {
                if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
                    // Won't have a field index
                    if (gepInst->getNumOperands() < 3)
                        continue;

                    if (const auto *gepStructType = llvm::dyn_cast<llvm::StructType>(gepInst->getSourceElementType())) {
                        for (auto j = 0; j < structTypes.size(); j++) {
                            auto structType = structTypes[j];
                            if (gepStructType != structType)
                                continue;

                            if (const auto *fieldIndexOperand = llvm::dyn_cast<llvm::ConstantInt>(gepInst->getOperand(2))) {
                                const auto fieldIndex = fieldIndexOperand->getZExtValue();
                                fieldUsages[structType].push_back(gepInst);
                                foundUsesInModule = true;
                                foundUsesInFunction = true;

                                llvm::errs() << TAB_STR << "Found use of: ST[" << j << "]";
                                llvm::errs() << "[" << fieldIndex << "](";

                                // Handle known types
                                const auto fieldType = structType->getElementType(fieldIndex);
                                auto isFieldTypeKnown = false;
                                for (auto k = 0; k < structTypes.size(); k++) {
                                    if (fieldType != structTypes[k])
                                        continue;
                                    llvm::errs() << "ST[" << k << "]";
                                    isFieldTypeKnown = true;
                                    break;
                                }
                                if (!isFieldTypeKnown)
                                    fieldType->print(llvm::errs(), true);

                                llvm::errs() << ")\n";
                            }
                        }
                    }
                }
            }

            if(!foundUsesInFunction)
                llvm::errs() << TAB_STR << "No field uses found in function\n";
        }

        if (!foundUsesInModule) {
            llvm::errs() << "No field uses found in module\n";
            return false;
        }

        llvm::errs() << fieldUsages.size() << "!!!\n";
        return true;
    }

    void enterModule() {
        llvm::errs() << "Entering Module: [" << module->getName() << "]\n";
    }

    bool exitModule() {
        llvm::errs() << "Exiting Module: [" << module->getName() << "]\n";
        if (!didWork) {
            llvm::errs() << TAB_STR << "Module Unchanged\n";
            return false;
        }

        llvm::errs() << TAB_STR << "LIST CHANGES HERE\n";
        return true;
    }
};

//     // Create the new constant value you want
//     // auto *newIndex = ConstantInt::get(CI->getIntegerType(), 1L);
//     // GEP->setOperand(GEP->getNumOperands() - 1, newIndex);
}

#pragma region LLVM Plugin

namespace {
struct ZippyPass : llvm::PassInfoMixin<ZippyPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
        return ZippyPassImpl(&M).tryRun() ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
    }
};
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "ZippyPass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::ModulePassManager &MPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
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

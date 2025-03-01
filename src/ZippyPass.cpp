#include "ModuleContext.hpp"

#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

using namespace llvm;

namespace {
    const std::string TAB_STR = "    ";

    class ZippyPassImpl {
        Module *module;
        bool didWork = false;

        std::vector<StructType*> structTypes;
        std::vector<Function*> functions;

        std::unordered_map<
            StructType*, std::unordered_map<Function*, std::vector<std::pair<GetElementPtrInst*, unsigned>>>
        > fieldUseMap;

        std::unordered_map<StructType*, std::vector<unsigned>> fieldCommonPrefixes;
        std::unordered_map<StructType*, std::vector<std::pair<Type*, unsigned>>> fieldLayoutRemaps;

    public:
        explicit ZippyPassImpl(Module *module): module(module) {
        }

        bool tryRun() {
            enterModule();

            if (!(findStructTypes() && findFunctions() && findFieldUsages())) {
                errs() << "No work in module\n";
                return exitModule();
            }

            if (!findFieldCommonPrefixes()) {
                errs() << "No better layouts found\n";
                return exitModule();
            }

            return exitModule();
        }

    private:
        void enterModule() {
            errs() << "Entering Module: [" << module->getName() << "]\n";
        }

        bool exitModule() {
            errs() << "Exiting Module: [" << module->getName() << "]\n";
            if (!didWork) {
                errs() << TAB_STR << "Module Unchanged\n";
                return false;
            }

            errs() << TAB_STR << "LIST CHANGES HERE\n";
            return true;
        }

        bool findStructTypes() {
            structTypes = module->getIdentifiedStructTypes();
            if (structTypes.empty()) {
                errs() << "No struct types found\n";
                return false;
            } else {
                errs() << "Found (" << structTypes.size() << ") struct types:\n";

                for (auto i = 0; i < structTypes.size(); i++) {
                    const auto structType = structTypes[i];
                    errs() << TAB_STR << "ST[" << i << "] -> ";
                    structType->print(errs(), true);
                    errs() << "\n";
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
                errs() << "No functions found\n";
                return false;
            } else {
                errs() << "Found (" << functions.size() << ") functions:\n";

                for (auto i = 0; i < functions.size(); i++) {
                    const auto function = functions[i];
                    errs() << TAB_STR << "F[" << i << "] -> ";
                    errs() << (function->hasName() ? function->getName() : "?") << " = ";
                    function->getFunctionType()->print(errs(), true);
                    errs() << "\n";
                }
                return true;
            }
        }

        bool findFieldUsages() {
            auto foundUsesInModule = false;
            for (auto i = 0; i < functions.size(); i++) {
                const auto function = functions[i];
                auto foundUsesInFunction = false;
                errs() << "Field usages in: F[" << i << "]\n";


                for (auto &inst: function->getEntryBlock()) {
                    if (auto *gepInst = dyn_cast<GetElementPtrInst>(&inst)) {
                        // Won't have a field index
                        if (gepInst->getNumOperands() < 3)
                            continue;

                        if (const auto *gepStructType = dyn_cast<StructType>(gepInst->getSourceElementType())) {
                            for (auto j = 0; j < structTypes.size(); j++) {
                                auto structType = structTypes[j];
                                if (gepStructType != structType)
                                    continue;

                                if (const auto *fieldIndexOperand = dyn_cast<ConstantInt>(
                                    gepInst->getOperand(2))) {
                                    const auto fieldIndex = fieldIndexOperand->getZExtValue();
                                    fieldUseMap[structType][function].emplace_back(gepInst, fieldIndex);
                                    foundUsesInModule = true;
                                    foundUsesInFunction = true;

                                    errs() << TAB_STR << "ST[" << j << "]";
                                    errs() << "[" << fieldIndex << "](";

                                    // Handle known types
                                    const auto fieldType = structType->getElementType(fieldIndex);
                                    auto isFieldTypeKnown = false;
                                    for (auto k = 0; k < structTypes.size(); k++) {
                                        if (fieldType != structTypes[k])
                                            continue;
                                        errs() << "ST[" << k << "]";
                                        isFieldTypeKnown = true;
                                        break;
                                    }
                                    if (!isFieldTypeKnown)
                                        fieldType->print(errs(), true);

                                    errs() << ")\n";
                                }
                            }
                        }
                    }
                }

                if (!foundUsesInFunction)
                    errs() << TAB_STR << "No field uses found in function\n";
            }

            if (!foundUsesInModule) {
                errs() << "No field uses found in module\n";
                return false;
            }


            for (auto i = 0; i < structTypes.size(); i++) {
                auto structType = structTypes[i];
                if (!fieldUseMap.count(structType))
                    continue;
                auto fieldUseLists = fieldUseMap[structType];

                errs() << "Field usages of: ST[" << i << "]\n";
                for (auto j = 0; j < functions.size(); j++) {
                    auto function = functions[j];
                    if (!fieldUseLists.count(function))
                        continue;
                    auto fieldUsages = fieldUseLists[function];

                    errs() << TAB_STR << "F[" << j << "]: (|";
                    for (auto [gepInst, fieldIndex]: fieldUsages)
                        errs() << fieldIndex << "|";
                    errs() << ")\n";
                }
            }
            return true;
        }


        bool findFieldCommonPrefixes() {
            auto foundPrefixes = false;
            for (auto i = 0; i < structTypes.size(); i++) {
                auto structType = structTypes[i];
                if (!fieldUseMap.count(structType))
                    continue;
                auto fieldUseLists = fieldUseMap[structType];

                std::vector<unsigned> commonPrefix;
                for (auto j = 0; j < structType->getNumElements(); j++) {
                }

                for (auto [function, fieldUsages]: fieldUseLists) {
                }
            }
            return foundPrefixes;
        }
    };

    //     // Create the new constant value you want
    //     // auto *newIndex = ConstantInt::get(CI->getIntegerType(), 1L);
    //     // GEP->setOperand(GEP->getNumOperands() - 1, newIndex);
}

#pragma region LLVM Plugin

namespace Zippy {
    struct ZippyPass : PassInfoMixin<ZippyPass> {
        static PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            auto ctx = ModuleContext::init(M, AM);
            return ctx.finish();
        }
    };
}

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

#pragma endregion

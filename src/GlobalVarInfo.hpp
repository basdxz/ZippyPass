#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    class GlobalVarInfo {
        GlobalVariable globalVar;

        explicit GlobalVarInfo(const GlobalVariable globalVar): globalVar(globalVar) {}

    public:
        static std::vector<GlobalVarInfo> collect(llvm::Module &M) {
            llvm::errs() << "Collecting Global Variables\n";
            std::vector<GlobalVarInfo> globalVarInfos;
            for (auto &globalVarRaw: M.globals()) {
                const GlobalVariable globalVar = {&globalVarRaw};
                // Non-struct globals are fully ignored
                if (!globalVar.isStructType()) continue;
                // Log variable name
                llvm::errs() << TAB_STR << globalVar;
                // Check for initializer
                if (!globalVar.isZeroInit()) {
                    globalVarInfos.push_back(GlobalVarInfo(globalVar));
                } else {
                    llvm::errs() << " - Zero init, skipped";
                }
                llvm::errs() << "\n";
            }
            if (globalVarInfos.empty()) {
                llvm::errs() << "No Global Variables collected\n\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Global Variables\n\n", globalVarInfos.size());
            }
            return std::move(globalVarInfos);
        }

        GlobalVariable& getGlobalVar() {
            return globalVar;
        }

        Type getValueType() const {
            return {globalVar.ptr->getValueType()};
        }

        void remap(const std::vector<unsigned> &remapTable) {
            // Get old initializer
            const auto oldInitializer = llvm::dyn_cast<llvm::ConstantStruct>(globalVar.ptr->getInitializer());
            if (!oldInitializer) llvm_unreachable("Old Initializer for Global Variable absent.");
            // Validate Operand count in remap table AND initializer
            const auto numOperands = oldInitializer->getNumOperands();
            if (numOperands != remapTable.size()) llvm_unreachable("Global Variable Operands don't match Remap Table");
            // Compute new Operands
            std::vector<llvm::Constant*> newOperands(numOperands);
            for (auto i = 0; i < numOperands; ++i) {
                newOperands[i] = oldInitializer->getOperand(remapTable[i]);
            }
            // Create new Initializer
            const auto structType = llvm::dyn_cast<llvm::StructType>(getValueType().ptr);
            if (!structType) llvm_unreachable("Global Variable Type is not a Struct");
            const auto newInitializer = llvm::ConstantStruct::get(structType, newOperands);
            // Apply new initializer
            globalVar.ptr->setInitializer(newInitializer);
        }
    };
}

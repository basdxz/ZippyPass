#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    class FunctionInfo {
        Function function;
        std::vector<GetElementPtrInstRef> gepRefs;

    public:
        explicit FunctionInfo(const Function function): function(function) {}

        Function getFunction() const {
            return function;
        }

        bool collectGepInsts() {
            // Entry block will start at the first function
            for (auto &inst: function.ptr->getEntryBlock()) {
                // Collect only gep instructions
                auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst);
                if (!gepInst) continue;
                // Fewer than three operands mean that this gep is just an array index
                if (gepInst->getNumOperands() < 3) continue;
                // Our source element will only ever be struct types
                if (!llvm::isa<llvm::StructType>(gepInst->getSourceElementType())) continue;

                // Check for uses by a `StoreInst` as that would indicate writing
                auto isWrite = false;
                for (const auto user: gepInst->users())
                    if (isWrite = llvm::isa<llvm::StoreInst>(user); isWrite) break;

                // Put it in the big list
                gepRefs.emplace_back(gepInst, isWrite);
            }
            // Returns false if none have been found
            return !gepRefs.empty();
        }

        // Returned values have a `StructType` as their source element, no arrays
        std::vector<GetElementPtrInstRef> getGepRefs() {
            return gepRefs;
        }
    };
}

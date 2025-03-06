#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    class FunctionInfo {
        Function function;
        std::vector<GetElementPtrInstRef> gepRefs;

        // Tracks the number of gepRefs that we are actually using.
        // Expected to be the same number as the size of gepRefs or lower,
        // if a struct that a struct that the gep is referring to is not our own.
        unsigned numUsedGepRefs;

        explicit FunctionInfo(const Function function): function(function), numUsedGepRefs(0) {
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
        }

    public:
        static std::vector<FunctionInfo> collect(llvm::Module &M) {
            llvm::errs() << "Collecting Functions\n";
            std::vector<FunctionInfo> functionInfos;
            for (auto &functionRaw: M.functions()) {
                Function function{&functionRaw};
                llvm::errs() << TAB_STR << function;
                if (!function.isDefined()) {
                    llvm::errs() << " - Not defined, skipped\n";
                    continue;
                }
                FunctionInfo functionInfo(function);
                const auto &gepRefs = functionInfo.getGepRefs();
                if (gepRefs.empty()) {
                    llvm::errs() << " - No struct references, skipped\n";
                    continue;
                }
                functionInfos.push_back(functionInfo);
                llvm::errs() << llvm::format(" - Found [%d] GEPs\n", gepRefs.size());
            }
            if (functionInfos.empty()) {
                llvm::errs() << "No Functions collected\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Functions\n", functionInfos.size());
            }
            return std::move(functionInfos);
        }

        const Function &getFunction() const {
            return function;
        }

        unsigned getNumUsedGepRefs() const {
            return numUsedGepRefs;
        }

        void incrementUsedGepRefs(const unsigned foundUses) {
            numUsedGepRefs += foundUses;
        }

        const std::vector<GetElementPtrInstRef> &getGepRefs() const {
            return gepRefs;
        }
    };
}

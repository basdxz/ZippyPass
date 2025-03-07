#pragma once

#include <set>

#include "ZippyCommon.hpp"
#include "GetElementPtrRef.hpp"

namespace Zippy {
    class FunctionInfo {
        Function function;

        // Shared pointers are used as GetElementPtrRef has children structs
        std::vector<std::shared_ptr<GetElementPtrRef>> gepRefs;

        // Tracks the number of gepRefs that we are actually using.
        // Expected to be the same number as the size of gepRefs or lower,
        // if a struct that a struct that the gep is referring to is not our own.
        unsigned numUsedGepRefs;

        explicit FunctionInfo(const Function function): function(function), numUsedGepRefs(0) {
            const auto ptr = function.ptr;
            // TODO: I suspect this will capture duplicate instructions...
            for (llvm::inst_iterator I = inst_begin(ptr), E = inst_end(ptr); I != E; ++I) {
                llvm::Instruction *inst = &*I;

                // Handle explicit GEP instructions
                if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(inst)) {
                    processGEPInst(gepInst);
                }

                for (auto &operand: inst->operands()) {
                    if (auto *gepExpr = llvm::dyn_cast<llvm::GEPOperator>(operand)) {
                        // Convert GEPOperator to GEPInst for consistent handling
                        processGEPOperator(gepExpr, inst);
                    }
                }
            }
        }

        void processGEPInst(llvm::GetElementPtrInst *gepInst) {
            // Skip if fewer than three operands (array indexing only)
            if (gepInst->getNumOperands() < 3) return;

            // Our source element needs to be a struct type
            if (!llvm::isa<llvm::StructType>(gepInst->getSourceElementType())) return;

            // Check for uses by a `StoreInst` as that would indicate writing
            auto isWrite = false;
            for (const auto user: gepInst->users())
                if (isWrite = llvm::isa<llvm::StoreInst>(user); isWrite) break;

            // Add it to the collection
            gepRefs.push_back(std::make_shared<GetElementPtrInstRef>(gepInst, isWrite));
        }

        void processGEPOperator(llvm::GEPOperator *gepOp, llvm::Instruction *parentInst) {
            // Skip if fewer than three operands (array indexing only)
            if (gepOp->getNumOperands() < 3) return;

            // Our source element needs to be a struct type
            if (!llvm::isa<llvm::StructType>(gepOp->getSourceElementType())) return;

            // Check if this is used for writing by looking at parent instruction
            auto isWrite = false;
            if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(parentInst)) {
                // If the GEP is the destination of a store, it's a write
                if (storeInst->getPointerOperand() == gepOp) {
                    isWrite = true;
                }
            }

            // Add it to the collection
            gepRefs.push_back(std::make_shared<GetElementPtrOpRef>(gepOp, isWrite));
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

        const std::vector<std::shared_ptr<GetElementPtrRef>> &getGepRefs() const {
            return gepRefs;
        }
    };
}

#pragma once

#include <set>

#include "ZippyCommon.hpp"
#include "GetElementPtrRef.hpp"

namespace Zippy {
    class FunctionInfo {
        Function function;

        // Shared pointers are used as GetElementPtrRef has children structs
        std::vector<std::shared_ptr<GetElementPtrRef>> gepRefs;

        std::shared_ptr<llvm::LoopInfo> loopInfo;

        // Tracking for found refs
        unsigned numGEPInst;
        unsigned numGEPOps;
        unsigned numDirectRefs;

        // Tracks the number of gepRefs that we are actually using.
        unsigned numUsedGepRefs;

        explicit FunctionInfo(const Function function): function(function), numGEPInst(0), numGEPOps(0),
                                                        numDirectRefs(0), numUsedGepRefs(0) {
            const auto ptr = function.ptr;
            // Scan all instructions in the function, we do it like it's done in the spec
            for (llvm::inst_iterator I = inst_begin(ptr), E = inst_end(ptr); I != E; ++I) {
                llvm::Instruction *inst = &*I;

                // Only handle Load and Store Instructions
                if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(inst)) {
                    processLoadOrStore(inst, loadInst->getPointerOperand(), GetElementPtrRef::LOAD);
                } else if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(inst)) {
                    processLoadOrStore(inst, storeInst->getPointerOperand(), GetElementPtrRef::STORE);
                } else if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(inst)) {
                    processGEPInst(gepInst, GetElementPtrRef::UNKNOWN);
                }
            }
            if (gepRefs.empty()) return;

            const auto domTree = std::make_unique<llvm::DominatorTree>(*function.ptr);
            // LoopInfo Takes ownership of the dominator tree here
            loopInfo = std::make_shared<llvm::LoopInfo>(*domTree);

            // Print debug info about loops found
            unsigned loopCount = 0;
            for (const auto &loopRef: *loopInfo) {
                loopCount += 1 + loopRef->getSubLoops().size();
            }

            //TODO: Fix lazy print formatting...
            if (loopCount > 0) {
                llvm::errs() << TAB_STR << "Function " << function.ptr->getName()
                        << ": found " << loopCount << " loops\n";
            } else {
                llvm::errs() << TAB_STR << "No Loops found\n";
            }
        }

        void processLoadOrStore(llvm::Instruction *inst, llvm::Value *ptrOperand,
                                const GetElementPtrRef::RefType type) {
            // Branching based on known ways the actual field reference could be used
            if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(ptrOperand)) {
                // As a GEP Instruction
                processGEPInst(gepInst, type);
            } else if (auto *gepOp = llvm::dyn_cast<llvm::GEPOperator>(ptrOperand)) {
                // As a GEP Operator
                processGEPOperator(inst, gepOp, type);
            } else if (const auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(ptrOperand)) {
                // As a Direct Reference (only handling global variables for now)
                processDirectRef(inst, globalVar, type);
            }
        }

        void processGEPInst(llvm::GetElementPtrInst *gepInst, const GetElementPtrRef::RefType type) {
            // Skip if fewer than three operands (array indexing only)
            if (gepInst->getNumOperands() < 3) return;
            // Our source element needs to be a struct type
            if (!llvm::isa<llvm::StructType>(gepInst->getSourceElementType())) return;
            // Add it to the collection
            gepRefs.push_back(std::make_shared<GetElementPtrInstRef>(gepInst, type));
            numGEPInst++;
        }

        void processGEPOperator(llvm::Instruction *inst, llvm::GEPOperator *gepOp,
                                const GetElementPtrRef::RefType type) {
            // Skip if fewer than three operands (array indexing only)
            if (gepOp->getNumOperands() < 3) return;
            // Our source element needs to be a struct type
            if (!llvm::isa<llvm::StructType>(gepOp->getSourceElementType())) return;
            // Add it to the collection
            gepRefs.push_back(std::make_shared<GetElementPtrOpRef>(inst, gepOp, type));
            numGEPOps++;
        }

        void processDirectRef(llvm::Instruction *inst, const llvm::GlobalVariable *globalVar,
                              const GetElementPtrRef::RefType type) {
            // Check for Struct Type
            auto *structTy = llvm::dyn_cast<llvm::StructType>(globalVar->getValueType());
            if (!structTy) return;
            // Add Direct Reference
            gepRefs.push_back(std::make_shared<DirectStructRef>(inst, structTy, type));
            numDirectRefs++;
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
                if (functionInfo.getGepRefs().empty()) {
                    llvm::errs() << " - No struct references, skipped\n";
                    continue;
                }
                functionInfos.push_back(functionInfo);
                llvm::errs() << llvm::format(" - Found Refs: I:[%d] O:[%d] D:[%d]\n", functionInfo.numGEPInst,
                                             functionInfo.numGEPOps, functionInfo.numDirectRefs);
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

        std::shared_ptr<llvm::LoopInfo> getLoopInfo() const {
           return loopInfo;
        }
    };
}

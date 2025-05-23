#pragma once


#include "ZippyCommon.hpp"
#include "GetElementPtrRef.hpp"
#include "IntrinsicInstRef.hpp"

namespace Zippy {
    // Only here to reduce verbosity
    typedef llvm::SmallPtrSet<llvm::GetElementPtrInst*, 8> GEPInstSet;

    class FunctionInfo {
        Function function;

        // Shared pointers are used as GetElementPtrRef has children structs
        std::vector<std::shared_ptr<GetElementPtrRef>> gepRefs;
        // Intrinsic instructions such as memcpy or memset
        std::vector<std::shared_ptr<IntrinsicInstRef>> intrinsicInsts;

        std::shared_ptr<llvm::LoopInfo> loopInfo;

        // Tracking for found refs
        unsigned numGEPInst;
        unsigned numGEPOps;
        unsigned numDirectRefs;

        // Tracks the number of gepRefs that we are actually using.
        unsigned numUsedGepRefs;

        explicit FunctionInfo(const Function function): function(function), numGEPInst(0), numGEPOps(0),
                                                        numDirectRefs(0), numUsedGepRefs(0) {
            GEPInstSet foundGEPs;

            const auto ptr = function.ptr;
            // Scan all instructions in the function, we do it like it's done in the spec
            for (llvm::inst_iterator I = inst_begin(ptr), E = inst_end(ptr); I != E; ++I) {
                llvm::Instruction *inst = &*I;

                if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(inst)) {
                    // Handles: `load`
                    processLoadOrStore(foundGEPs, inst, loadInst->getPointerOperand(), GetElementPtrRef::LOAD);
                } else if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(inst)) {
                    // Handles: `store`
                    processLoadOrStore(foundGEPs, inst, storeInst->getPointerOperand(), GetElementPtrRef::STORE);
                } else if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(inst)) {
                    // Handles: `getelementptr`
                    processGEPInst(foundGEPs, gepInst, GetElementPtrRef::UNKNOWN);
                } else if (auto *memCpyInst = llvm::dyn_cast<llvm::MemCpyInst>(inst)) {
                    // Handles: `@llvm.memcpy.p0.*`
                    intrinsicInsts.push_back(std::make_shared<MemCpyInstRef>(memCpyInst));
                } else if (auto *memSetInst = llvm::dyn_cast<llvm::MemSetInst>(inst)) {
                    // Handles: `@llvm.memset.p0.*`
                    intrinsicInsts.push_back(std::make_shared<MemSetInstRef>(memSetInst));
                }
            }
            if (gepRefs.empty() && intrinsicInsts.empty()) return;

            const auto domTree = std::make_unique<llvm::DominatorTree>(*function.ptr);
            // LoopInfo Takes ownership of the dominator tree here
            loopInfo = std::make_shared<llvm::LoopInfo>(*domTree);

            // Print debug info about loops found
            unsigned loopCount = 0;
            for (const auto &loopRef: *loopInfo) {
                loopCount += 1 + loopRef->getSubLoops().size();
            }

            if (loopCount > 0) {
                llvm::errs() << "\n" << TAB_STR_2 << llvm::format("Found: [%d] Loops", loopCount);
            }
        }

        void processLoadOrStore(GEPInstSet &foundGEPs, llvm::Instruction *inst, llvm::Value *ptrOperand,
                                const GetElementPtrRef::RefType type) {
            // Branching based on known ways the actual field reference could be used
            if (auto *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(ptrOperand)) {
                // As a GEP Instruction
                processGEPInst(foundGEPs, gepInst, type);
            } else if (auto *gepOp = llvm::dyn_cast<llvm::GEPOperator>(ptrOperand)) {
                // As a GEP Operator
                processGEPOperator(inst, gepOp, type);
            } else if (const auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(ptrOperand)) {
                // As a Direct Reference (only handling global variables for now)
                processDirectRef(inst, globalVar, type);
            }
        }

        void processGEPInst(GEPInstSet &foundGEPs, llvm::GetElementPtrInst *gepInst, GetElementPtrRef::RefType type) {
            // Avoid duplicates
            if (!foundGEPs.insert(gepInst).second) return;
            // Skip if fewer than three operands (array indexing only)
            if (gepInst->getNumOperands() < 3) return;
            // This would be true if we used nested anonymous structs, but that is unsupported
            if (gepInst->getNumOperands() != 3)
                llvm_unreachable("Expected three or fewer GEP operands");
            // Our source element needs to be a struct type
            if (!llvm::isa<llvm::StructType>(gepInst->getSourceElementType())) return;
            // Attempt to resolve type if unknown
            if (type == GetElementPtrRef::UNKNOWN)
                type = resolveRefType(gepInst);
            // Add it to the collection
            gepRefs.push_back(std::make_shared<GetElementPtrInstRef>(gepInst, type));
            numGEPInst++;
        }

        static GetElementPtrRef::RefType resolveRefType(llvm::GetElementPtrInst *gepInst) {
            for (const auto gepUser: gepInst->users()) {
                if (llvm::isa<llvm::LoadInst>(gepUser)) return GetElementPtrRef::LOAD;
                if (llvm::isa<llvm::StoreInst>(gepUser)) return GetElementPtrRef::STORE;
                if (llvm::isa<llvm::CallInst>(gepUser)) return GetElementPtrRef::CALL;
                if (auto *next = llvm::dyn_cast<llvm::GetElementPtrInst>(gepUser)) {
                    auto type = resolveRefType(next);
                    if (type != GetElementPtrRef::UNKNOWN) return type;
                }
            }
            return GetElementPtrRef::UNKNOWN;
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
                // Don't mention undefined functions at all
                if (!function.isDefined()) continue;
                llvm::errs() << TAB_STR << function;
                FunctionInfo functionInfo(function);
                if (functionInfo.getGepRefs().empty()) {
                    if (functionInfo.intrinsicInsts.empty()) {
                        llvm::errs() << " - No struct references, skipped\n";
                        continue;
                    }
                }
                functionInfos.push_back(functionInfo);
                llvm::errs() << "\n" << TAB_STR_2 << llvm::format("Found Refs: I:[%d] O:[%d] D:[%d] C[%d]\n",
                                                                 functionInfo.numGEPInst,
                                                                 functionInfo.numGEPOps, functionInfo.numDirectRefs,
                                                                 functionInfo.intrinsicInsts.size());
            }
            if (functionInfos.empty()) {
                llvm::errs() << "No Functions collected\n\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Functions\n\n", functionInfos.size());
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

        const std::vector<std::shared_ptr<IntrinsicInstRef>> &getIntrinsicInsts() const {
            return intrinsicInsts;
        }

        std::shared_ptr<llvm::LoopInfo> getLoopInfo() const {
            return loopInfo;
        }
    };
}

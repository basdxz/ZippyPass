#pragma once

#include "ZippyCommon.hpp"
#include "FieldInfo.hpp"
#include "FunctionInfo.hpp"
#include "GlobalVarInfo.hpp"

namespace Zippy {
    class StructInfo {
        // TODO: This is a guess and will "generally" be correct, it's wrong when geps are generated in a chained way.
        static constexpr unsigned FIELD_IDX = 2;

        StructType structType;
        std::vector<FieldInfo> fieldInfos;
        unsigned numFieldInfos;
        bool isPacked;
        std::vector<GlobalVarInfo> globalVarInfos;
        std::vector<std::shared_ptr<IntrinsicInstRef>> intrinsicRefs;

        std::vector<unsigned> remapTable;
        unsigned sumFieldUses = 0;

        llvm::TypeSize initialSize = llvm::TypeSize::getZero();
        llvm::TypeSize currentSize = llvm::TypeSize::getZero();

        explicit StructInfo(const StructType structType, const llvm::DataLayout &DL): structType(structType),
            initialSize(DL.getTypeAllocSize(structType.ptr)),
            currentSize(initialSize) {
            numFieldInfos = structType.ptr->getNumElements();
            isPacked = structType.ptr->isPacked();

            // Collect all the elements from this struct early
            fieldInfos.reserve(numFieldInfos);
            for (auto i = 0; i < numFieldInfos; i++) {
                fieldInfos.emplace_back(DL, structType, i);
            }

            // Pre-generate identity remap table
            remapTable.reserve(numFieldInfos);
            for (auto i = 0; i < numFieldInfos; ++i) {
                remapTable.emplace_back(i);
            }
        }

    public:
        static std::vector<StructInfo> collect(const llvm::Module &M, const llvm::DataLayout &DL) {
            llvm::errs() << "Collecting Structs\n";
            std::vector<StructInfo> structInfos;
            for (const auto structTy: M.getIdentifiedStructTypes()) {
                const StructType structType{structTy};
                // Skip unnamed structs
                if (!structType.ptr->hasName()) continue;
                // Skip structs with less than two elements
                if (structType.ptr->getNumElements() < 2) continue;
                // TODO: Currently skipping 'timespec' structs, but should have a more robust marking system
                if (structType.ptr->getName() == "struct.timespec") continue;

                auto structInfo = StructInfo(structType, DL);
                llvm::errs() << TAB_STR << structInfo.getStructType() << "\n";
                structInfos.push_back(structInfo);
            }
            if (structInfos.empty()) {
                llvm::errs() << "No Structs collected\n\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Structs\n\n", structInfos.size());
            }
            return std::move(structInfos);
        }

        const StructType &getStructType() const {
            return structType;
        }

        unsigned getSumFieldUses() const {
            return sumFieldUses;
        }

        /**
         * Can be re-ordered externally, but pretty please don't break it...
         */
        std::vector<FieldInfo> &getFieldInfos() {
            return fieldInfos;
        }

        llvm::TypeSize getInitialSize() {
            return initialSize;
        }

        llvm::TypeSize getCurrentSize() {
            return currentSize;
        }

        unsigned collectFieldUses(FunctionInfo &functionInfo) {
            unsigned foundUses = 0;
            for (const auto &gepRef: functionInfo.getGepRefs()) {
                // Check if the source element is this struct
                if (!gepRef->isSameSourceStructType(structType)) continue;
                // Get the operand and validate that it is indeed, a `ConstantInt`
                const auto *fieldIndexOperand = llvm::dyn_cast<llvm::ConstantInt>(gepRef->getOperand(FIELD_IDX));
                if (!fieldIndexOperand) continue;

                // Get the field index and add the usage
                const auto fieldIndex = fieldIndexOperand->getZExtValue();
                fieldInfos[fieldIndex].addUse(functionInfo.getLoopInfo(), gepRef, FIELD_IDX);

                // Track uses
                foundUses++;
            }
            sumFieldUses += foundUses;
            functionInfo.incrementUsedGepRefs(foundUses);

            // TODO: This is a quick fix for finding relevant mem copies, tidy later
            for (auto intrinsicRef: functionInfo.getIntrinsicInsts()) {
                if (intrinsicRef->getDstType() == structType.ptr)
                    intrinsicRefs.push_back(intrinsicRef);
            }

            return foundUses;
        }

        /**
         * This function inverts control flow, so that the actual sorting logic is top-level.
         */
        void computeFieldWeights(const std::function<void(FieldInfo &)> &func) {
            for (auto &fieldInfo: fieldInfos) {
                func(fieldInfo);
            }
        }

        unsigned collectGlobalVars(std::vector<GlobalVarInfo> &allGlobalVarInfos) {
            llvm::errs() << TAB_STR << "For Struct: ";
            structType.printName(llvm::errs());
            llvm::errs() << "\n";
            auto varsCollected = 0;
            for (auto &globalVarInfo: allGlobalVarInfos) {
                if (globalVarInfo.getValueType().ptr != structType.ptr) continue;
                globalVarInfos.push_back(globalVarInfo);
                llvm::errs() << TAB_STR_2 << "Collected: ";
                globalVarInfo.getGlobalVar().printName(llvm::errs());
                llvm::errs() << "\n";
                varsCollected++;
            }
            if (varsCollected == 0) {
                llvm::errs() << TAB_STR << "None collected\n";
            } else {
                llvm::errs() << TAB_STR << llvm::format("Collected [%d] Global Variables\n", varsCollected);
            }
            return varsCollected;
        }

        void normalizeWeights() {
            auto maxSizeWeight = 1.0F;
            auto maxLoadWeight = 1.0F;
            auto maxStoreWeight = 1.0F;
            auto maxLoopWeight = 1.0F;
            // Find maximum weights
            for (const auto &fieldInfo: fieldInfos) {
                maxSizeWeight = std::max(maxSizeWeight, fieldInfo.getSizeWeight());
                maxLoadWeight = std::max(maxLoadWeight, fieldInfo.getLoadWeight());
                maxStoreWeight = std::max(maxStoreWeight, fieldInfo.getStoreWeight());
                maxLoopWeight = std::max(maxLoopWeight, fieldInfo.getLoopWeight());
            }
            // Normalized weights (0, 1)
            for (auto &fieldInfo: fieldInfos) {
                fieldInfo.setSizeWeight(fieldInfo.getSizeWeight() / maxSizeWeight);
                fieldInfo.setLoadWeight(fieldInfo.getLoadWeight() / maxLoadWeight);
                fieldInfo.setStoreWeight(fieldInfo.getStoreWeight() / maxStoreWeight);
                fieldInfo.setLoopWeight(fieldInfo.getLoopWeight() / maxLoopWeight);
            }
        }

        bool applyTransform(const llvm::DataLayout &DL) {
            updateTargetIndices();
            // Early return if no work was done
            if (!remapFields()) return false;
            // Update the body and current size
            updateBody();
            updateCurrentSize(DL);
            // Apply alignment
            for (auto i = 0; i < numFieldInfos; i++) {
                fieldInfos[i].applyAlign(calculateFieldAlignment(DL, structType.ptr, i));
            }
            // Remap global variables
            for (auto &globalVarInfo: globalVarInfos) {
                globalVarInfo.remap(remapTable);
            }
            // Update intrinsic references
            for (const auto intrinsicRef: intrinsicRefs) {
                intrinsicRef->setTypeSize(currentSize);
            }
            // Print debug info
            llvm::errs() << TAB_STR << "Transformation Result:\n";
            llvm::errs() << TAB_STR_2 << llvm::format("Initial size: [%d] Current Size: [%d]\n",
                                                      initialSize.getKnownMinValue(), currentSize.getKnownMinValue());
            for (const auto &fieldInfo: fieldInfos) {
                llvm::errs() << TAB_STR_2 << fieldInfo << "\n";
            }
            return true;
        }

    private:
        void updateTargetIndices() {
            for (auto i = 0; i < numFieldInfos; i++) {
                auto &fieldInfo = fieldInfos[i];
                remapTable[i] = fieldInfo.getCurrentIndex();
                fieldInfo.setTargetIndex(i);
            }
        }

        bool remapFields() {
            auto didWork = false;
            // Apply the remap to each field
            for (auto i = 0; i < numFieldInfos; i++) {
                didWork |= fieldInfos[i].applyRemap();
            }
            return didWork;
        }

        void updateBody() {
            // Create and populating the new struct body
            std::vector<llvm::Type*> newBody;
            newBody.reserve(numFieldInfos);
            for (const auto &fieldInfo: fieldInfos) {
                newBody.push_back(fieldInfo.getType().ptr);
            }
            // Replace struct body
            structType.ptr->setBody(newBody, isPacked);
        }

        void updateCurrentSize(const llvm::DataLayout &DL) {
            // TODO: The only way I've found to properly calculate the correct size of a modified llvm::StructType,
            //       is to create a new instance. This seems to be because llvm::DataLayout caches this value.
            //       I would like a better way to do this, because this way does cause a tiny memory leak.
            const auto dummyStructType = llvm::StructType::create(structType.ptr->getContext());
            dummyStructType->setBody(structType.ptr->elements(), isPacked);
            currentSize = DL.getTypeAllocSize(dummyStructType);
        }
    };
}

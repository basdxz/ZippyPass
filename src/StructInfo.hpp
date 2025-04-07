#pragma once

#include "ZippyCommon.hpp"
#include "FieldInfo.hpp"
#include "FunctionInfo.hpp"
#include "GlobalVarInfo.hpp"

namespace Zippy {
    class StructInfo {
        // TODO: This is a guess and will "generally" be correct, it's wrong when geps are generated in a chained way.
        const unsigned FIELD_IDX = 2;

        StructType structType;
        std::vector<FieldInfo> fieldInfos;
        std::vector<GlobalVarInfo> globalVarInfos;
        std::vector<std::shared_ptr<IntrinsicInstRef>> intrinsicRefs;

        std::vector<unsigned> remapTable;
        unsigned sumFieldUses;

        llvm::TypeSize initialSize;
        llvm::TypeSize currentSize;

        explicit StructInfo(const StructType structType, const llvm::DataLayout &DL): structType(structType),
            sumFieldUses(0), initialSize(llvm::TypeSize::getZero()), currentSize(llvm::TypeSize::getZero()) {
            // Collect all the elements from this struct early
            const auto numElements = structType.ptr->getNumElements();
            fieldInfos.reserve(numElements);

            for (auto i = 0; i < numElements; i++) {
                fieldInfos.emplace_back(structType.getElementType(i), i,
                                        calculateFieldAlignment(DL, structType.ptr, i));
            }

            // Pre-generate identity remap table
            remapTable.reserve(numElements);
            for (auto i = 0; i < numElements; ++i) {
                remapTable.emplace_back(i);
            }

            initialSize = DL.getTypeAllocSize(structType.ptr);
            currentSize = initialSize;
        }

    public:
        static std::vector<StructInfo> collect(const llvm::Module &M, const llvm::DataLayout &DL) {
            llvm::errs() << "Collecting Structs\n";
            std::vector<StructInfo> structInfos;
            for (const auto structType: M.getIdentifiedStructTypes()) {
                auto structInfo = StructInfo(StructType{structType}, DL);
                structInfos.push_back(structInfo);
                llvm::errs() << TAB_STR << structInfo.getStructType() << "\n";
            }
            if (structInfos.empty()) {
                llvm::errs() << "No Structs collected\n";
            } else {
                llvm::errs() << llvm::format("Collected [%d] Structs\n", structInfos.size());
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
                llvm::errs() << TAB_STR << TAB_STR << "Collected: ";
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

        void updateTargetIndices() {
            llvm::errs() << "Updating Target Indicies\n";

            const auto numFieldInfos = fieldInfos.size();
            for (auto i = 0; i < numFieldInfos; i++) {
                auto &fieldInfo = fieldInfos[i];
                remapTable[i] = fieldInfo.getCurrentIndex();
                fieldInfo.setTargetIndex(i);
            }

            for (int i = 0; i < numFieldInfos; ++i) {
                llvm::errs() << llvm::format("%d->%d\n", i, remapTable[i]);
            }
        }

        bool applyRemap() {
            auto didWork = false;
            const auto numFieldInfos = fieldInfos.size();

            for (auto i = 0; i < numFieldInfos; i++) {
                didWork |= fieldInfos[i].applyRemap();
            }

            // Early return if no work was done
            if (!didWork) return false;

            // Creating and populating the new struct body
            std::vector<llvm::Type*> newBody;
            newBody.reserve(numFieldInfos);
            for (const auto &fieldInfo: fieldInfos) {
                newBody.push_back(fieldInfo.getType().ptr);
            }
            // Replace struct body
            structType.ptr->setBody(newBody, structType.ptr->isPacked());

            // Remapping global variables
            for (auto &globalVarInfo: globalVarInfos) {
                globalVarInfo.remap(remapTable);
            }
            return true;
        }

        bool applyAlign(const llvm::DataLayout &DL) {
            auto didWork = false;
            const auto numFieldInfos = fieldInfos.size();
            for (auto i = 0; i < numFieldInfos; i++) {
                didWork |= fieldInfos[i].applyAlign(calculateFieldAlignment(DL, structType.ptr, i));
            }
            return didWork;
        }

        void updateIntrinsicRefs(const llvm::DataLayout &DL) {
            obligatoryMemoryLeak(DL);
            for (const auto intrinsicRef: intrinsicRefs)
                intrinsicRef->setTypeSize(currentSize);
        }

    private:
        void obligatoryMemoryLeak(const llvm::DataLayout &DL) {
            const auto dummyStrucType = llvm::StructType::create(structType.ptr->getContext());
            dummyStrucType->setBody(structType.ptr->elements(), dummyStrucType->isPacked());
            currentSize = DL.getTypeAllocSize(dummyStrucType);
        }
    };
}

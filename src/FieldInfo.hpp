#pragma once

#include "ZippyCommon.hpp"
#include "cmath"
#include "GetElementPtrRef.hpp"

namespace Zippy {
    class FieldUse {
        std::shared_ptr<llvm::LoopInfo> loopInfo;
        std::shared_ptr<GetElementPtrRef> gepRef;
        // Operator index is separate from the field index, as GEPs may reference a nested field (not implemented atm)
        unsigned operandIndex;

    public:
        FieldUse(const std::shared_ptr<llvm::LoopInfo> &loopInfo, const std::shared_ptr<GetElementPtrRef> &gepRef,
                 const unsigned operandIndex): loopInfo(loopInfo), gepRef(gepRef), operandIndex(operandIndex) {}

        bool isWrite() {
            return gepRef->isWrite();
        }

        uint64_t getFieldIndex() const {
            return gepRef->getOperand(operandIndex)->getZExtValue();
        }

        void setFieldIndex(const uint64_t index) const {
            const auto oldOperand = gepRef->getOperand(operandIndex);
            // Early return if no work needs to be done, happens due to some duplication jank
            if (oldOperand->getZExtValue() == index) return;

            // In LLVM, for some reason to create a `ConstantInt`, we need a reference to the type?
            // But the only way I've found to reliably get the reference, has been to have the old value on hand.
            // Thus, we need to *read* the original value, get the type, then use that to create the *new* type.
            const auto operandType = oldOperand->getIntegerType();
            gepRef->setOperand(operandIndex, llvm::ConstantInt::get(operandType, index));
        }

        void setAlignment(const llvm::Align alignment) {
            gepRef->setAlignment(alignment);
        }

        // New method to access the GEP reference
        std::shared_ptr<GetElementPtrRef> getGepRef() const {
            return gepRef;
        }

        unsigned computeLoopDepth() const {
            // The parent should always be present, but rather throw a clear error than segfault
            const auto parent = gepRef->getInst()->getParent();
            if (!parent)
                llvm_unreachable("FieldUse parent cannot be null");
            // Find a loop if exists, and get its depth. Otherwise, assume not in a loop.
            const auto loop = loopInfo->getLoopFor(parent);
            return loop ? loop->getLoopDepth() : 0;
        }
    };

    class FieldInfo {
        Type type;
        llvm::Align initialAlign;
        llvm::Align currentAlign;
        llvm::TypeSize storeSize;
        llvm::TypeSize allocSize;
        std::vector<FieldUse> uses;

        unsigned numLoads = 0;
        unsigned numStores = 0;

        float loopAccessWeight = 1.0F;
        float totalWeight = 0.0F;

        unsigned initialIndex;
        unsigned currentIndex;
        unsigned targetIndex;

    public:
        explicit FieldInfo(const llvm::DataLayout &DL, const StructType structType, const unsigned index):
            type(structType.getElementType(index)),
            initialAlign(type.getABIAlign(DL)),
            currentAlign(initialAlign),
            storeSize(type.getStoreSize(DL)),
            allocSize(type.getAllocSize(DL)),
            initialIndex(index),
            currentIndex(index),
            targetIndex(index) {}

        Type getType() const {
            return type;
        }

        llvm::Align getInitialAlign() const {
            return initialAlign;
        }

        llvm::Align getCurrentAlign() const {
            return currentAlign;
        }

        llvm::TypeSize getStoreSize() const {
            return storeSize;
        }

        llvm::TypeSize getAllocSize() const {
            return allocSize;
        }

        unsigned getNumLoads() const {
            return numLoads;
        }

        unsigned getNumStores() const {
            return numStores;
        }

        unsigned getSumLoadStores() const {
            return numLoads + numStores;
        }

        // Original index as found in the source code
        unsigned getInitialIndex() const {
            return initialIndex;
        }

        unsigned getCurrentIndex() const {
            return currentIndex;
        }

        unsigned getTargetIndex() const {
            return targetIndex;
        }

        void setTargetIndex(const unsigned idx) {
            targetIndex = idx;
        }

        void addUse(std::shared_ptr<llvm::LoopInfo> loopInfo, std::shared_ptr<GetElementPtrRef> gepRef,
                    const unsigned operandIndex) {
            uses.emplace_back(loopInfo, gepRef, operandIndex);
            switch (gepRef->getType()) {
                case GetElementPtrRef::LOAD:
                    ++numLoads;
                    break;
                case GetElementPtrRef::STORE:
                    ++numStores;
                    break;
                default:
                    break;
            }
        }

        const std::vector<FieldUse> &getUses() const {
            return uses;
        }

        void setLoopAccessWeight(const float weight) {
            loopAccessWeight = weight;
        }

        float getLoopAccessWeight() const {
            return loopAccessWeight;
        }

        void setTotalWeight(const float weight) {
            totalWeight = weight;
        }

        float getTotalWeight() const {
            return totalWeight;
        }

        bool applyRemap() {
            // Skip remap if index is the same
            if (currentIndex == targetIndex) return false;
            // Set current index to the target index
            currentIndex = targetIndex;
            // Skip work if field has no uses
            if (uses.empty()) return false;
            // Set all new field indices to target index
            for (auto use: uses)
                use.setFieldIndex(targetIndex);
            return true;
        }

        void applyAlign(const llvm::Align align) {
            if (currentAlign == align) return;
            for (auto use: uses)
                use.setAlignment(align);
        }

        void print(llvm::raw_ostream &out) const {
            // Append the index
            if (initialIndex == currentIndex) {
                out << llvm::format("Index: [%02d==%02d]", initialIndex, currentIndex);
            } else {
                out << llvm::format("Index: [%02d->%02d]", initialIndex, currentIndex);
            }
            out << " - ";
            // Append the alignment
            if (initialAlign == currentAlign) {
                out << llvm::format("Align: [%02d==%02d]", initialAlign.value(), currentAlign.value());
            } else {
                out << llvm::format("Align: [%02d->%02d]", initialAlign.value(), currentAlign.value());
            }
            out << " - ";
            // Store Size
            out << llvm::format("Store Size: [%d]", storeSize.getKnownMinValue());
            out << " - ";
            // Alloc Size
            out << llvm::format("Alloc Size: [%d]", allocSize.getKnownMinValue());
        }
    };
}

namespace llvm {
    inline raw_ostream &operator<<(raw_ostream &out, const Zippy::FieldInfo &fieldInfo) {
        fieldInfo.print(out);
        return out;
    }
}

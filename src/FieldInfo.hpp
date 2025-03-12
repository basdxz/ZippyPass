#pragma once

#include "ZippyCommon.hpp"
#include "GetElementPtrRef.hpp"

namespace Zippy {
    class FieldUse {
        std::shared_ptr<GetElementPtrRef> gepRef;
        // Operator index is separate from the field index, as GEPs may reference a nested field (not implemented atm)
        unsigned operandIndex;

    public:
        FieldUse(const std::shared_ptr<GetElementPtrRef> &gepRef,
                 const unsigned operandIndex): gepRef(gepRef),
                                               operandIndex(operandIndex) {}

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
    };

    class FieldInfo {
        Type type;
        std::vector<FieldUse> uses;

        unsigned numLoads;
        unsigned numStores;

        unsigned originalIndex;
        unsigned currentIndex;
        unsigned targetIndex;

    public:
        explicit FieldInfo(const Type type, const unsigned index): type(type), numLoads(0), numStores(0),
                                                                   originalIndex(index), currentIndex(index),
                                                                   targetIndex(index) {}

        const Type &getType() const {
            return type;
        }

        unsigned getNumLoads() const {
            return numLoads;
        }

        unsigned getNumStores() const {
            return numLoads;
        }

        unsigned getSumLoadStores() const {
            return numLoads + numStores;
        }

        // Original index as found in the source code
        unsigned getOriginalIndex() const {
            return originalIndex;
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

        void addUse(std::shared_ptr<GetElementPtrRef> gepRef, const unsigned operandIndex) {
            uses.emplace_back(gepRef, operandIndex);
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

        bool applyRemap() {
            // Skip remap if index is the same
            if (currentIndex == targetIndex) return false;
            // Set current index to the target index
            currentIndex = targetIndex;
            // Skip work if field has no uses
            if (uses.empty()) return false;
            // Set all new field indices to target index
            for (auto user: uses)
                user.setFieldIndex(targetIndex);
            return true;
        }
    };
}

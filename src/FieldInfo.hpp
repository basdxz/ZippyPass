#pragma once

#include "ZippyCommon.hpp"
#include "GetElementPtrRef.hpp"

namespace Zippy {
    struct FieldUse {
        std::shared_ptr<GetElementPtrRef> gepRef;
        unsigned operandIndex;

        FieldUse(const std::shared_ptr<GetElementPtrRef> &gepRef,
                 const unsigned operandIndex): gepRef(gepRef),
                                               operandIndex(operandIndex) {}

        uint64_t getFieldIndex() const {
            return gepRef->getOperand(operandIndex)->getZExtValue();
        }

        void setFieldIndex(const uint64_t index) const {
            // In LLVM, for some reason to create a `ConstantInt`, we need a reference to the type?
            // But the only way I've found to reliably get the reference, has been to have the old value on hand.
            // Thus, we need to *read* the original value, get the type, then use that to create the *new* type.
            const auto operandType = gepRef->getOperand(operandIndex)->getIntegerType();
            gepRef->setOperand(operandIndex, llvm::ConstantInt::get(operandType, index));
        }
    };

    class FieldInfo {
        Type type;
        std::vector<FieldUse> uses;

        unsigned originalIndex;
        unsigned currentIndex;
        unsigned targetIndex;

    public:
        explicit FieldInfo(const Type type, const unsigned index): type(type), originalIndex(index),
                                                                   currentIndex(index), targetIndex(index) {}

        const Type &getType() const {
            return type;
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
        }

        unsigned getNumUses() const {
            return uses.size();
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

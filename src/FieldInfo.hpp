#pragma once

#include "ZippyCommon.hpp"
#include <vector>

namespace Zippy {
    struct FieldUse {
        GetElementPtrInstRef gepRef;
        unsigned operandIndex;

        FieldUse(const GetElementPtrInstRef gepRef,
                 const unsigned operandIndex): gepRef(gepRef),
                                               operandIndex(operandIndex) {}

        uint64_t getFieldIndex() const {
            return gepRef.getOperand(operandIndex)->getZExtValue();
        }

        void setFieldIndex(const uint64_t index) const {
            // In LLVM, for some reason to create a `ConstantInt`, we need a reference to the type?
            // But the only way I've found to reliably get the reference, has been to have the old value on hand.
            // Thus, we need to *read* the original value, get the type, then use that to create the *new* type.
            const auto operandType = gepRef.getOperand(operandIndex)->getIntegerType();
            gepRef.setOperand(operandIndex, llvm::ConstantInt::get(operandType, index));
        }
    };

    class FieldInfo {
        Type type;
        std::vector<FieldUse> uses;

        unsigned currentIndex;
        unsigned targetIndex;

    public:
        FieldInfo(const Type type, const unsigned idx): type(type), currentIndex(idx), targetIndex(idx) {}

        Type getType() const {
            return type;
        }

        unsigned getCurrentIndex() const {
            return currentIndex;
        }

        unsigned getTargetIndex() const {
            return targetIndex;
        }

        void setTargetIndex(const unsigned targetIndex) {
            this->targetIndex = targetIndex;
        }

        void addUse(const GetElementPtrInstRef gepRef, const unsigned operandIndex) {
            uses.emplace_back(gepRef, operandIndex);
        }

        std::vector<FieldUse> &getUses() {
            return uses;
        }

        bool applyRemap() {
            // Skip remap if index is the same
            if (currentIndex == targetIndex) return false;
            // Skip remap if unused
            if (uses.empty()) return false;

            // Set all new field indices to target index
            for (auto user: uses)
                user.setFieldIndex(targetIndex);

            // Set current index to the target index
            currentIndex = targetIndex;
            return true;
        }
    };
}

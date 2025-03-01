#pragma once

#include "ZippyCommon.hpp"
#include <llvm/IR/Constants.h>
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
        FieldInfo(const unsigned idx, const Type type): type(type), currentIndex(idx), targetIndex(idx) {}

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

        void addUse(const FieldUse &use) {
            uses.push_back(use);
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

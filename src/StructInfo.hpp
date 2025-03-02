#pragma once

#include "ZippyCommon.hpp"
#include "FieldInfo.hpp"
#include "FunctionInfo.hpp"

namespace Zippy {
    class StructInfo {
        // TODO: This is a guess and will "generally" be correct, it's wrong when geps are generated in a chained way.
        const unsigned FIELD_IDX = 2;

        StructType structType;

        std::vector<FieldInfo> fields;
        unsigned sumFieldUses;

    public:
        explicit StructInfo(const StructType structType): structType(structType), sumFieldUses(0) {
            // Collect all the elements from this struct early
            const auto numElements = structType.ptr->getNumElements();
            for (auto i = 0; i < numElements; i++)
                fields.emplace_back(structType.getElementType(i), i);
        }

        StructType getStructType() const {
            return structType;
        }

        unsigned getSumFieldUses() const {
            return sumFieldUses;
        }

        unsigned collectFieldUses(FunctionInfo& functionInfo) {
            unsigned foundUses = 0;
            for (const auto &gepRef: functionInfo.getGepRefs()) {
                // Check if the source element is this struct
                if (!gepRef.isSameSourceStructType(structType)) continue;
                // Get the operand and validate that it is indeed, a `ConstantInt`
                const auto *fieldIndexOperand = llvm::dyn_cast<llvm::ConstantInt>(gepRef.ptr->getOperand(FIELD_IDX));
                if (!fieldIndexOperand) continue;

                // Get the field index and add the usage
                const auto fieldIndex = fieldIndexOperand->getZExtValue();
                fields[fieldIndex].addUse(gepRef, FIELD_IDX);

                // Track uses
                foundUses++;
            }
            sumFieldUses += foundUses;
            functionInfo.incrementUsedGepRefs(foundUses);
            return foundUses;
        }
    };
}

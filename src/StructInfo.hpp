#pragma once

#include "ZippyCommon.hpp"
#include "FieldInfo.hpp"
#include "FunctionInfo.hpp"

namespace Zippy {
    class StructInfo {
        // TODO: This is a guess and will "generally" be correct
        const unsigned FIELD_IDX = 2;

        StructType structType;

        std::vector<FieldInfo> fields;

    public:
        explicit StructInfo(const StructType structType): structType(structType) {}

        void initFieldInfo() {
            const auto numElements = structType.ptr->getNumElements();
            for (auto i = 0; i < numElements; i++)
                fields.emplace_back(structType.getElementType(i), i);
        }

        void collectUsages(FunctionInfo functionInfo) {
            for (const auto gepRef: functionInfo.getGepRefs()) {
                // Check if the source element is this struct
                if (gepRef.ptr->getSourceElementType() != structType.ptr) continue;
                // Get the operand and validate that it is indeed, a `ConstantInt`
                const auto *fieldIndexOperand = llvm::dyn_cast<llvm::ConstantInt>(gepRef.ptr->getOperand(FIELD_IDX));
                if (!fieldIndexOperand) continue;

                // Get the field index and add the usage
                const auto fieldIndex = fieldIndexOperand->getZExtValue();
                fields[fieldIndex].addUse({gepRef, FIELD_IDX});
            }
        }
    };
}

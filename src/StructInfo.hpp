#pragma once

#include "ZippyCommon.hpp"
#include "FieldInfo.hpp"
#include "FunctionInfo.hpp"

namespace Zippy {
    class StructInfo {
        // TODO: This is a guess and will "generally" be correct, it's wrong when geps are generated in a chained way.
        const unsigned FIELD_IDX = 2;

        StructType structType;

        std::vector<FieldInfo> fieldInfos;
        unsigned sumFieldUses;

    public:
        explicit StructInfo(const StructType structType): structType(structType), sumFieldUses(0) {
            // Collect all the elements from this struct early
            const auto numElements = structType.ptr->getNumElements();
            for (auto i = 0; i < numElements; i++)
                fieldInfos.emplace_back(structType.getElementType(i), i);
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
                fieldInfos[fieldIndex].addUse(gepRef, FIELD_IDX);

                // Track uses
                foundUses++;
            }
            sumFieldUses += foundUses;
            functionInfo.incrementUsedGepRefs(foundUses);
            return foundUses;
        }

        bool applyRemap() {
            auto didWork = false;
            const auto numFieldInfos = fieldInfos.size();
            for (auto i = 0; i < numFieldInfos; i++) {
                didWork |= fieldInfos[i].applyRemap(i);
            }

            // Early return if no work was done
            if (!didWork) return false;

            // Replace struct body
            std::vector<llvm::Type *> newBody;
            for (auto field: fieldInfos) {
                newBody.push_back(field.getType().ptr);
            }
            structType.ptr->setBody(newBody, structType.ptr->isPacked());
            return true;
        }
    };
}

#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    class IntrinsicInstRef {
    protected:
        llvm::Type *dstType;

    public:
        virtual ~IntrinsicInstRef() = default;

        // TODO: No implementations of this method don't account for current size
        virtual void setTypeSize(llvm::TypeSize typeSize) = 0;

        llvm::Type *getDstType() {
            return dstType;
        }

    protected:
        explicit IntrinsicInstRef(const llvm::CallBase *callBase, const unsigned dstArgIndex) {
            dstType = nullptr;
            for (auto &use: callBase->getArgOperand(dstArgIndex)->uses()) {
                if (const auto allocInst = llvm::dyn_cast<llvm::AllocaInst>(use)) {
                    dstType = allocInst->getAllocatedType();
                    return;
                }
                if (const auto gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(use)) {
                    dstType = gepInst->getSourceElementType();
                    return;
                }
                if (const auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(use)) {
                    dstType = globalVar->getValueType();
                    return;
                }
            }
            llvm_unreachable("Destination type is null, failed to implicitly find");
        }
    };

    class MemCpyInstRef final : public IntrinsicInstRef {
        llvm::MemCpyInst *ptr;

    public:
        explicit MemCpyInstRef(llvm::MemCpyInst *ptr)
            : IntrinsicInstRef(ptr, 0), ptr(ptr) {}

        void setTypeSize(const llvm::TypeSize typeSize) override {
            const auto baseLen = llvm::cast<llvm::ConstantInt>(ptr->getArgOperand(2));
            ptr->setOperand(2, llvm::ConstantInt::get(baseLen->getIntegerType(), typeSize));
        }
    };

    class MemSetInstRef final : public IntrinsicInstRef {
        llvm::MemSetInst *ptr;

    public:
        explicit MemSetInstRef(llvm::MemSetInst *ptr)
            : IntrinsicInstRef(ptr, 0), ptr(ptr) {}

        void setTypeSize(const llvm::TypeSize typeSize) override {
            const auto baseLen = llvm::cast<llvm::ConstantInt>(ptr->getArgOperand(2));
            ptr->setOperand(2, llvm::ConstantInt::get(baseLen->getIntegerType(), typeSize));
        }
    };
}

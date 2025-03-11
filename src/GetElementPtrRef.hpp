#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    struct GetElementPtrRef {
        bool isWrite;

        virtual ~GetElementPtrRef() = default;

        virtual llvm::ConstantInt *getOperand(unsigned operandIndex) const = 0;
        virtual void setOperand(unsigned operandIndex, llvm::ConstantInt *operand) = 0;
        virtual llvm::Type *getSourceType() const = 0;

        virtual bool isOperator() const {
            return false;
        }

        virtual bool isInstruction() const {
            return false;
        }

        bool isSameSourceStructType(StructType const &structType) const {
            return structType.ptr == getSourceType();
        }

    protected:
        GetElementPtrRef(const bool isWrite): isWrite(isWrite) {}
    };

    /**
     * Contains GEP Instructions
     */
    struct GetElementPtrInstRef final : GetElementPtrRef {
        llvm::GetElementPtrInst *ptr;

        GetElementPtrInstRef(llvm::GetElementPtrInst *ptr, const bool isWrite)
            : GetElementPtrRef(isWrite), ptr(ptr) {}

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return llvm::cast<llvm::ConstantInt>(ptr->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) override {
            ptr->setOperand(operandIndex, operand);
        }

        llvm::Type *getSourceType() const override {
            return ptr->getSourceElementType();
        }
    };

    /**
     * Contains GEP Operators, which occur in nested instructions
     */
    struct GetElementPtrOpRef final : GetElementPtrRef {
        llvm::GEPOperator *ptr;

        GetElementPtrOpRef(llvm::GEPOperator *ptr, const bool isWrite)
            : GetElementPtrRef(isWrite), ptr(ptr) {}

        bool isOperator() const override {
            return true;
        }

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return llvm::cast<llvm::ConstantInt>(ptr->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) override {
            ptr->setOperand(operandIndex, operand);
        }

        llvm::Type *getSourceType() const override {
            return ptr->getSourceElementType();
        }
    };

    /**
     * Handles direct struct access (field 0) without GEP instructions
     */
    struct DirectStructRef final : GetElementPtrRef {
        llvm::Instruction *ptr;
        llvm::StructType *structType;
        llvm::ConstantInt *operand; // TODO: Technically, this never updates correctly.

        DirectStructRef(llvm::Instruction *ptr, llvm::StructType *structType, const bool isWrite)
            : GetElementPtrRef(isWrite), ptr(ptr), structType(structType) {
            // TODO: This *may* not provide the correct TYPE of integer, but we only use it to read the value back
            operand = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ptr->getContext()), 0);
        }

        bool isInstruction() const override {
            return true;
        }

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            assert(operandIndex == 2); // Magic constant woooo!
            return operand;
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) override {
            assert(operandIndex == 2); // Magic constant woooo!
            const unsigned fieldIndex = operand->getZExtValue();
            // Avoid changing the IR if no change is needed
            if (fieldIndex == this->operand->getZExtValue()) return;

            // Create a new GEP Instruction
            llvm::IRBuilder irBuilder(ptr);
            const auto gepInst = irBuilder.CreateStructGEP(structType, getPointerOperand(), fieldIndex);

            // Apply the new GEP Instruction
            setGEPInstOperand(gepInst);

            // Update the operand
            this->operand = operand;
        }

        llvm::Type *getSourceType() const override {
            return structType;
        }

    private:
        llvm::Value *getPointerOperand() const {
            if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(ptr)) {
                return loadInst->getPointerOperand();
            }
            if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(ptr)) {
                return storeInst->getPointerOperand();
            }
            llvm_unreachable("Expected LoadInst or StoreInst");
        }

        void setGEPInstOperand(llvm::Value *gepInst) const {
            if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(ptr)) {
                loadInst->setOperand(loadInst->getPointerOperandIndex(), gepInst);
                return;
            }
            if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(ptr)) {
                storeInst->setOperand(storeInst->getPointerOperandIndex(), gepInst);
                return;
            }
            llvm_unreachable("Expected LoadInst or StoreInst");
        }
    };
}

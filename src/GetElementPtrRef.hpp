#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    struct GetElementPtrRef {
        llvm::Value *ptr;
        bool isWrite;

        virtual ~GetElementPtrRef() = default;

        virtual llvm::ConstantInt *getOperand(unsigned operandIndex) const = 0;
        virtual void setOperand(unsigned operandIndex, llvm::ConstantInt *operand) const = 0;
        virtual llvm::Type *getSourceType() const = 0;
        virtual bool isOperator() const = 0;

        bool isSameSourceStructType(StructType const &structType) const {
            return structType.ptr == getSourceType();
        }

    protected:
        GetElementPtrRef(llvm::Value *ptr, const bool isWrite): ptr(ptr), isWrite(isWrite) {}

        static llvm::ConstantInt *asConstInt(llvm::Value *value) {
            return llvm::cast<llvm::ConstantInt>(value);
        }
    };

    /**
      * Contains GEP Instructions
      */
    struct GetElementPtrInstRef final : GetElementPtrRef {
        GetElementPtrInstRef(llvm::GetElementPtrInst *ptr, const bool isWrite)
            : GetElementPtrRef(ptr, isWrite) {}

        bool isOperator() const override {
            return true;
        }

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return asConstInt(realPtr()->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) const override {
            realPtr()->setOperand(operandIndex, operand);
        }

        llvm::Type *getSourceType() const override {
            return realPtr()->getSourceElementType();
        }

    private:
        llvm::GetElementPtrInst *realPtr() const {
            return llvm::cast<llvm::GetElementPtrInst>(ptr);
        }
    };

    /**
  * Contains GEP Operators, which occur in nested instructions
  */
    struct GetElementPtrOpRef final : GetElementPtrRef {
        GetElementPtrOpRef(llvm::GEPOperator *ptr, const bool isWrite)
            : GetElementPtrRef(ptr, isWrite) {}

        bool isOperator() const override {
            return true;
        }

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return llvm::cast<llvm::ConstantInt>(realPtr()->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) const override {
            realPtr()->setOperand(operandIndex, operand);
        }

        llvm::Type *getSourceType() const override {
            return realPtr()->getSourceElementType();
        }

    private:
        llvm::GEPOperator *realPtr() const {
            return llvm::cast<llvm::GEPOperator>(ptr);
        }
    };
}

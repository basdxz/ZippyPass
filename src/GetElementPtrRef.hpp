#pragma once

#include "ZippyCommon.hpp"

namespace Zippy {
    class GetElementPtrRef {
    public:
        enum RefType {
            UNKNOWN,
            LOAD,
            STORE,
            CALL
        };

    protected:
        RefType type;

    public:
        virtual ~GetElementPtrRef() = default;

        virtual llvm::ConstantInt *getOperand(unsigned operandIndex) const = 0;
        virtual void setOperand(unsigned operandIndex, llvm::ConstantInt *operand) = 0;
        virtual void setAlignment(llvm::Align alignment) = 0;
        virtual llvm::Type *getSourceType() const = 0;
        virtual llvm::Instruction *getInst() const = 0;

        virtual RefType getType() const {
            return type;
        }

        virtual bool isWrite() const {
            return type == STORE;
        }

        virtual bool isRead() const {
            return type == LOAD;
        }

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
        explicit GetElementPtrRef(const RefType type): type(type) {}
    };

    /**
     * Contains GEP Instructions
     */
    class GetElementPtrInstRef final : public GetElementPtrRef {
        llvm::GetElementPtrInst *ptr;

    public:
        GetElementPtrInstRef(llvm::GetElementPtrInst *ptr, const RefType type)
            : GetElementPtrRef(type), ptr(ptr) {}

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return llvm::cast<llvm::ConstantInt>(ptr->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) override {
            ptr->setOperand(operandIndex, operand);
        }

        void setAlignment(const llvm::Align alignment) override {
            setAlignment(alignment, ptr);
        }

        llvm::Type *getSourceType() const override {
            return ptr->getSourceElementType();
        }

        llvm::Instruction *getInst() const override {
            return ptr;
        }

    private:
        static void setAlignment(const llvm::Align alignment, llvm::GetElementPtrInst *gepInst) {
            // We need to find **any** references we can reach and update the alignment
            for (const auto gepUser: gepInst->users()) {
                if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(gepUser)) {
                    loadInst->setAlignment(alignment);
                    continue;
                }
                if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(gepUser)) {
                    storeInst->setAlignment(alignment);
                    continue;
                }
                if (auto *next = llvm::dyn_cast<llvm::GetElementPtrInst>(gepUser)) {
                    setAlignment(alignment, next);
                }
            }
        }
    };

    /**
     * Contains GEP Operators, which occur in nested instructions
     */
    class GetElementPtrOpRef final : public GetElementPtrRef {
        llvm::Instruction *instPtr;
        llvm::GEPOperator *ptr;

    public:
        GetElementPtrOpRef(llvm::Instruction *instPtr, llvm::GEPOperator *ptr, const RefType type)
            : GetElementPtrRef(type), instPtr(instPtr), ptr(ptr) {}

        bool isOperator() const override {
            return true;
        }

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const override {
            return llvm::cast<llvm::ConstantInt>(ptr->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) override {
            ptr->setOperand(operandIndex, operand);
        }

        void setAlignment(const llvm::Align alignment) override {
            if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(instPtr)) {
                loadInst->setAlignment(alignment);
            } else if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(instPtr)) {
                storeInst->setAlignment(alignment);
            }
        }

        llvm::Type *getSourceType() const override {
            return ptr->getSourceElementType();
        }

        llvm::Instruction *getInst() const override {
            return instPtr;
        }
    };

    /**
     * Handles direct struct access (field 0) without GEP instructions
     */
    class DirectStructRef final : public GetElementPtrRef {
        llvm::Instruction *ptr;
        llvm::StructType *structType;
        llvm::GEPOperator *gepInst;
        llvm::ConstantInt *operand;
    public:
        DirectStructRef(llvm::Instruction *ptr, llvm::StructType *structType, const RefType type)
            : GetElementPtrRef(type), ptr(ptr), structType(structType) {

            // Creates an IR Builder
            llvm::IRBuilder irBuilder(ptr);
            // Creates a GEP, note that '12345' is an evil random number to *avoid* over-eager constant folding
            auto gep = irBuilder.CreateConstInBoundsGEP2_32(structType, getPointerOperand(), 12345, 0);

            // Actually apply the operand into the instruction unconditionally
            if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(ptr)) {
                loadInst->setOperand(loadInst->getPointerOperandIndex(), gep);
            } else if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(ptr)) {
                storeInst->setOperand(storeInst->getPointerOperandIndex(), gep);
            } else {
                llvm_unreachable("Expected LoadInst or StoreInst");
            }
            gepInst = llvm::cast<llvm::GEPOperator>(gep);
            operand = llvm::cast<llvm::ConstantInt>(gepInst->getOperand(2));

            // This completes the evil hack, which sets the 'array index' of the value back to zero.
            gepInst->setOperand(1, irBuilder.getInt32(0));
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

            // Apply Operand
            gepInst->setOperand(2, operand);

            // Update the operand
            this->operand = operand;
        }

        void setAlignment(const llvm::Align alignment) override {
            if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(ptr)) {
                loadInst->setAlignment(alignment);
            } else if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(ptr)) {
                storeInst->setAlignment(alignment);
            }
            llvm_unreachable("Expected LoadInst or StoreInst");
        }

        llvm::Type *getSourceType() const override {
            return structType;
        }

        llvm::Instruction *getInst() const override {
            return ptr;
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
    };
}

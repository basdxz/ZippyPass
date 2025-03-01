#pragma once

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/Constants.h>

namespace Zippy {
    const std::string NO_VAL_NAME_STR = "???";
    const std::string TAB_STR = "    ";

    struct Type {
        llvm::Type *ptr;
    };

    struct StructType {
        llvm::StructType *ptr;

        void printName(llvm::raw_ostream &OS) const {
            OS << (ptr->hasName() ? ptr->getName().ltrim("struct.") : NO_VAL_NAME_STR);
        }

        Type getElementType(const unsigned index) const {
            return {ptr->getElementType(index)};
        }
    };

    struct Function {
        llvm::Function *ptr;

        void printName(llvm::raw_ostream &OS) const {
            OS << (ptr->hasName() ? ptr->getName() : NO_VAL_NAME_STR);
        }
    };

    struct GetElementPtrInstRef {
        llvm::GetElementPtrInst *ptr;
        // Is GEP result is used in a store
        bool isWrite;

        GetElementPtrInstRef(llvm::GetElementPtrInst *ptr,
                             const bool isWrite): ptr(ptr), isWrite(isWrite) {}

        llvm::ConstantInt *getOperand(const unsigned operandIndex) const {
            return llvm::cast<llvm::ConstantInt>(ptr->getOperand(operandIndex));
        }

        void setOperand(const unsigned operandIndex, llvm::ConstantInt *operand) const {
            ptr->setOperand(operandIndex, operand);
        }
    };
}


/**
 * Using the LLVM name space, as these functions apply to `llvm::raw_ostream`
 */
namespace llvm {
    inline raw_ostream &operator<<(raw_ostream &out, const Zippy::StructType structType) {
        const auto ptr = structType.ptr;

        structType.printName(out);
        out << "{";

        const auto elements = ptr->elements();
        const auto elementCount = elements.size();
        for (auto i = 0; i < elementCount; i++) {
            const auto element = elements[i];

            if (const auto structTypeElement = dyn_cast<StructType>(element)) {
                Zippy::StructType{structTypeElement}.printName(out);
            } else {
                element->print(out, true);
            }

            if (i + 1 < elementCount) {
                errs() << ", ";
            }
        }

        out << "}";
        return out;
    }

    inline raw_ostream &operator<<(raw_ostream &out, const Zippy::Function function) {
        const auto ptr = function.ptr;

        const auto ret = ptr->getReturnType();
        ret->print(errs(), true);
        errs() << " ";
        function.printName(out);
        out << "(";

        const auto argCount = ptr->arg_size();
        for (auto i = 0; i < argCount; i++) {
            const auto arg = ptr->getArg(i);
            if (const auto argType = arg->getType(); argType->isPointerTy()) {
                // Pointer types resolved by value were passed by values and are not pointers in the source code
                if (ptr->hasParamAttribute(i, Attribute::ByVal)) {
                    const auto byValType = ptr->getParamByValType(i);
                    if (const auto structType = dyn_cast<StructType>(byValType)) {
                        Zippy::StructType{structType}.printName(out);
                    } else {
                        byValType->print(out, true);
                    }
                    goto resolved;
                }
                // Tries to resolve the type of this argument from an "opaque" pointer,
                // since if the type is an opaque pointer, it's probably a pointer to a struct.
                //
                // The general usage pattern in IR looks like the following:
                // - Store the argument using the `StoreInst`, where the 2nd argument is the destination
                // - Load it using the `LoadInst`, where the location is the destination
                // - Use the destination in a `GetElementPtrInst`, where the type can now be resolved
                for (const auto argUser: arg->users()) {
                    // Find the store instruction
                    const auto *storeInst = dyn_cast<StoreInst>(argUser);
                    if (!storeInst) continue;

                    auto const storeDst = storeInst->getOperand(1);
                    for (const auto storeDstUser: storeDst->users()) {
                        // Find the load instruction
                        const auto *loadInst = dyn_cast<LoadInst>(storeDstUser);
                        if (!loadInst) continue;

                        for (const auto loadDstUser: loadInst->users()) {
                            // Find the gep instruction
                            const auto *gepInst = dyn_cast<GetElementPtrInst>(loadDstUser);
                            if (!gepInst) continue;

                            const auto gepType = gepInst->getSourceElementType();
                            // Resolve the type name if it is a struct
                            if (auto *structType = dyn_cast<StructType>(gepType)) {
                                Zippy::StructType{structType}.printName(out);
                            } else {
                                gepType->print(out, true);
                            }
                            out << "*";
                            goto resolved;
                        }
                    }
                }

                // Fallback if we couldn't resolve the pointer name
                out << Zippy::NO_VAL_NAME_STR << "*";
            } else {
                argType->print(out, true);
            }
        resolved:
            if (i + 1 < argCount) {
                errs() << ", ";
            }
        }

        out << ")";
        return out;
    }
}

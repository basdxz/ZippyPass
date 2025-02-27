#ifndef STRUCTCONTEXT_HPP
#define STRUCTCONTEXT_HPP

#include "ModuleContext.hpp"

namespace Zippy {
    struct FieldUsage {
        llvm::GetElementPtrInst *gepInst;

        void setIndex(const uint64_t index) const {
            gepInst->setOperand(2, llvm::ConstantInt::get(getIndexRaw()->getIntegerType(), index));
        }

        uint64_t getIndex() const {
            return getIndexRaw()->getZExtValue();
        }

        llvm::ConstantInt *getIndexRaw() const {
            return cast<llvm::ConstantInt>(gepInst->getOperand(2));
        }
    };

    class StructContext {
    public:
        explicit StructContext(ModuleContext &ctx,
                               llvm::StructType *structType): ctx(ctx), structType(structType) {
        }

    private:
        ModuleContext &ctx;
        llvm::StructType *structType;

        std::vector<FieldUsage> usages;
    };
}
#endif

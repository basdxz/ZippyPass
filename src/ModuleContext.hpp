#ifndef MODULECONTEXT_HPP
#define MODULECONTEXT_HPP

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

namespace Zippy {
    class ModuleContext {
    public:
        explicit ModuleContext(llvm::Module &M,
                               llvm::ModuleAnalysisManager &AM): M(M), AM(AM) {
        }

    private:
        llvm::Module &M;
        llvm::ModuleAnalysisManager &AM;

        bool wasModified = false;

    public:
        std::vector<llvm::StructType *> structTypes;
        std::vector<llvm::Function *> functions;

        int findStructTypes() {
            structTypes = M.getIdentifiedStructTypes();
            return structTypes.size();
        }

        int findFunctionDeclarations() {
            for (auto &function: M.functions())
                if (!function.isDeclaration())
                    functions.push_back(&function);
            return functions.size();
        }

        void markModified() {
            wasModified = true;
        }

        llvm::PreservedAnalyses finish() {
            return wasModified ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
        }
    };
}
#endif

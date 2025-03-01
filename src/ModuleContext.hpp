#pragma once

#include "ZippyCommon.hpp"
#include "FunctionInfo.hpp"
#include "FieldInfo.hpp"
#include "StructInfo.hpp"

namespace Zippy {
    class ModuleContext {
        enum State {
            UNKNOWN,
            HAS_WORK,
            HAS_NO_WORK,
            WAS_MODIFIED
        };

        explicit ModuleContext(llvm::Module &M,
                               llvm::ModuleAnalysisManager &AM): M(M), AM(AM) {
        }

        llvm::Module &M;
        llvm::ModuleAnalysisManager &AM;

        std::vector<StructType> _structTypes;
        std::vector<Function> _functions;

        State state = UNKNOWN;

    public:
        static ModuleContext init(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
            auto ctx = ModuleContext(M, AM);

            for (const auto structType: M.getIdentifiedStructTypes()) {
                ctx._structTypes.push_back({structType});
            }

            for (auto &function: M.functions()) {
                if (!function.isDeclaration()) {
                    ctx._functions.push_back({&function});
                }
            }

            llvm::errs() << "Structs: \n";
            for (const auto &structType: ctx.structTypes()) {
                llvm::errs() << TAB_STR << structType << "\n";
            }

            llvm::errs() << "Functions: \n";
            for (const auto &function: ctx.functions()) {
                llvm::errs() << TAB_STR << function << "\n";
            }

            ctx.state = ctx._structTypes.empty() || ctx._functions.empty() ? HAS_NO_WORK : HAS_WORK;
            return ctx;
        }

        bool hasWork() {
            return state == HAS_WORK || state == WAS_MODIFIED;
        }

        void markModified() {
            assert(hasWork());
            state = WAS_MODIFIED;
        }

        std::vector<StructType> &structTypes() {
            return _structTypes;
        }

        std::vector<Function> &functions() {
            return _functions;
        }

        llvm::PreservedAnalyses finish() {
            return state == WAS_MODIFIED ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
        }
    };
}

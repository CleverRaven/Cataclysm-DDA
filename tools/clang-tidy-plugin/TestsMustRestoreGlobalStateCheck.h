#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H

#include <unordered_set>

#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"

namespace clang
{
class CompilerInstance;

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class TestsMustRestoreGlobalStateCheck : public ClangTidyCheck
{
    public:
        TestsMustRestoreGlobalStateCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}

        void registerPPCallbacks( CompilerInstance &Compiler ) override;
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;

        void setIsTestFile() {
            is_test_file_ = true;
        }
    private:
        bool is_test_file_ = false;
        std::unordered_set<const NamedDecl *> restored_decls_;

        struct AssignmentToGlobal {
            const BinaryOperator *assignment;
            const NamedDecl *lhsDecl;
        };

        std::vector<AssignmentToGlobal> suspicious_assignments_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H

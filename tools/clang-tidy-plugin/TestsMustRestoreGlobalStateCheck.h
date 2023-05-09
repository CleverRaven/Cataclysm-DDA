#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H

#include <unordered_set>

#include <llvm/ADT/StringRef.h>

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>
#include "Utils.h"

namespace clang
{
class CompilerInstance;

namespace tidy
{
class ClangTidyContext;

namespace cata
{

struct RestoredDecl {
    const FunctionDecl *function;
    const NamedDecl *variable;

    bool operator==( const RestoredDecl &other ) const {
        return function == other.function && variable == other.variable;
    }
};

} // namespace cata
} // namespace tidy
} // namespace clang

namespace std
{
template<>
struct hash<clang::tidy::cata::RestoredDecl> {
    std::size_t operator()( const clang::tidy::cata::RestoredDecl &r ) const noexcept {
        return clang::tidy::cata::HashCombine( r.function, r.variable );
    }
};
} // namespace std

namespace clang
{
namespace tidy
{
namespace cata
{

class TestsMustRestoreGlobalStateCheck : public ClangTidyCheck
{
    public:
        TestsMustRestoreGlobalStateCheck( StringRef Name, ClangTidyContext *Context );

        void registerPPCallbacks( const SourceManager &, Preprocessor *, Preprocessor * ) override;
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;

        void setIsTestFile() {
            is_test_file_ = true;
        }
    private:
        bool is_test_file_ = false;

        std::unordered_set<RestoredDecl> restored_decls_;

        struct AssignmentToGlobal {
            const BinaryOperator *assignment;
            RestoredDecl lhsDecl;
        };

        std::vector<AssignmentToGlobal> suspicious_assignments_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTSMUSTRESTOREGLOBALSTATECHECK_H

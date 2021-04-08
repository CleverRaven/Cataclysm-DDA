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
        hash<const clang::FunctionDecl *> function_hash;
        hash<const clang::NamedDecl *> decl_hash;
        size_t result = function_hash( r.function );
        result ^= 0x9e3779b9 + ( result << 6 ) + ( result >> 2 );
        result ^= decl_hash( r.variable );
        return result;
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

        void registerPPCallbacks( CompilerInstance &Compiler ) override;
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

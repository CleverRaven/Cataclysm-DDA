#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"
#include "Utils.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

struct CallContext {
    const Expr *stmt;
    const NamedDecl *on;

    bool operator==( const CallContext &other ) const {
        return stmt == other.stmt && on == other.on;
    }
};

} // namespace cata
} // namespace tidy
} // namespace clang

namespace std
{
template<>
struct hash<clang::tidy::cata::CallContext> {
    std::size_t operator()( const clang::tidy::cata::CallContext &c ) const noexcept {
        return clang::tidy::cata::HashCombine( c.stmt, c.on );
    }
};
} // namespace std

namespace clang
{
namespace tidy
{
namespace cata
{

class UnsequencedCallsCheck : public ClangTidyCheck
{
    public:
        UnsequencedCallsCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;
    private:
        void CheckCall( const ast_matchers::MatchFinder::MatchResult & );
        std::unordered_map<CallContext, std::vector<const CXXMemberCallExpr *>> calls_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H

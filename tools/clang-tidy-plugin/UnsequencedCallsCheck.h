#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>
#include <unordered_map>

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>
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

struct CallWithASTContext {
    const CXXMemberCallExpr *call;
    ASTContext *context;

    bool operator==( const CallWithASTContext &other ) const {
        return call == other.call;
    }
    bool operator==( const Expr *other ) const {
        return call == other;
    }
    bool operator<( const CallWithASTContext &other ) const {
        return std::less<> {}( call, other.call );
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
        std::unordered_map<CallContext, std::vector<CallWithASTContext>> calls_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_UNSEQUENCEDCALLSCHECK_H

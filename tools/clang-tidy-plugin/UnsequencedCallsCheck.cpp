#include "UnsequencedCallsCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include <tuple>

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void UnsequencedCallsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxMemberCallExpr(
            on( declRefExpr( to( namedDecl().bind( "on" ) ) ) )
        ).bind( "memberCall" ),
        this
    );
}

// Keep walking up the AST so long as the nodes are expressions.  When we reach
// a non-expression, return the previous expression.
// This is a vague approximation to finding the AST node which represents
// everything between two sequence points.
static const Expr *getContainingStatement(
    const ast_matchers::MatchFinder::MatchResult &Result, const Expr *Node )
{
    for( const ast_type_traits::DynTypedNode &parent : Result.Context->getParents( *Node ) ) {
        if( const Expr *Candidate = parent.get<Expr>() ) {
            return getContainingStatement( Result, Candidate );
        }
    }

    return Node;
}

void UnsequencedCallsCheck::CheckCall( const MatchFinder::MatchResult &Result )
{
    const CXXMemberCallExpr *MemberCall =
        Result.Nodes.getNodeAs<CXXMemberCallExpr>( "memberCall" );
    const NamedDecl *On =
        Result.Nodes.getNodeAs<NamedDecl>( "on" );
    if( !MemberCall || !On ) {
        return;
    }

    const Expr *ContainingStatement = getContainingStatement( Result, MemberCall );

    CallContext context{ ContainingStatement, On };
    calls_[context].push_back( MemberCall );
}

void UnsequencedCallsCheck::onEndOfTranslationUnit()
{
    for( const std::pair<const CallContext, std::vector<const CXXMemberCallExpr *>> &p : calls_ ) {
        const CallContext &context = p.first;
        const std::vector<const CXXMemberCallExpr *> &calls = p.second;

        if( calls.size() < 2 ) {
            continue;
        }
        bool any_non_const = false;
        for( const CXXMemberCallExpr *member_call : calls ) {
            const CXXMethodDecl *method = member_call->getMethodDecl();
            if( !method->isConst() ) {
                any_non_const = true;
                break;
            }
        }

        if( any_non_const ) {
            diag(
                context.stmt->getBeginLoc(),
                "Unsequenced calls to member functions of %0, at least one of which is non-const."
            ) << context.on;
        }
    }
}

void UnsequencedCallsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckCall( Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang

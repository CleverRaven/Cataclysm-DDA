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
static const Expr *getContainingSequenceStatement(
    const ast_matchers::MatchFinder::MatchResult &Result, const Expr *Node )
{
    for( const ast_type_traits::DynTypedNode &parent : Result.Context->getParents( *Node ) ) {
        if( parent.get<ConditionalOperator>() ) {
            return Node;
        }
        if( const Expr *Candidate = parent.get<Expr>() ) {
            return getContainingSequenceStatement( Result, Candidate );
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

    const Expr *ContainingStatement = getContainingSequenceStatement( Result, MemberCall );

    CallContext context{ ContainingStatement, On };
    calls_[context].push_back( MemberCall );
}

static bool IsEffectivelyConstCall( const CXXMemberCallExpr *Call )
{
    const CXXMethodDecl *method = Call->getMethodDecl();
    if( method->isConst() ) {
        return true;
    }
    StringRef Name = method->getName();
    if( Name == "begin" || Name == "end" || Name == "find" ) {
        return true;
    }
    return false;
}

void UnsequencedCallsCheck::onEndOfTranslationUnit()
{
    for( std::pair<const CallContext, std::vector<const CXXMemberCallExpr *>> &p : calls_ ) {
        const CallContext &context = p.first;
        std::vector<const CXXMemberCallExpr *> &calls = p.second;
        std::sort( calls.begin(), calls.end() );
        calls.erase( std::unique( calls.begin(), calls.end() ), calls.end() );

        if( calls.size() < 2 ) {
            continue;
        }
        bool any_non_const = false;
        for( const CXXMemberCallExpr *member_call : calls ) {
            if( !IsEffectivelyConstCall( member_call ) ) {
                any_non_const = true;
                break;
            }
        }

        if( any_non_const ) {
            diag(
                context.stmt->getBeginLoc(),
                "Unsequenced calls to member functions of %0, at least one of which is non-const."
            ) << context.on;

            for( const CXXMemberCallExpr *member_call : calls ) {
                const CXXMethodDecl *method = member_call->getMethodDecl();
                if( IsEffectivelyConstCall( member_call ) ) {
                    diag( member_call->getBeginLoc(), "Call to const member function %0",
                          DiagnosticIDs::Note ) << method;
                } else {
                    diag( member_call->getBeginLoc(), "Call to non-const member function %0",
                          DiagnosticIDs::Note ) << method;
                }
            }
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

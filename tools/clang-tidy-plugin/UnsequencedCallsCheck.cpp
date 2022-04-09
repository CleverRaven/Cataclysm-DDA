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
#include <unordered_set>

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
// a non-expression or an expressions whose subexpressions we know to be
// sequenced (like a short-circuiting logical operator), return the previous expression.
// This is a vague approximation to finding the AST node which represents
// everything between two sequence points.
static const Expr *GetContainingSequenceStatement( ASTContext *Context, const Expr *Node )
{
    for( const DynTypedNode &parent : Context->getParents( *Node ) ) {
        if( parent.get<ConditionalOperator>() ) {
            return Node;
        }
        if( parent.get<InitListExpr>() ) {
            return Node;
        }
        if( const BinaryOperator *op = parent.get<BinaryOperator>() ) {
            if( op->isLogicalOp() ) {
                return Node;
            }
        }
        if( const CXXConstructExpr *construct = parent.get<CXXConstructExpr>() ) {
            if( construct->isListInitialization() ) {
                return Node;
            }
        }
        if( const Expr *Candidate = parent.get<Expr>() ) {
            return GetContainingSequenceStatement( Context, Candidate );
        }
    }

    return Node;
}

// Keep walking up the AST so long as the nodes are expressions.  When we reach
// a non-expression, return the previous expression.
// This is a vague approximation to finding the AST node which represents
// everything between two sequence points.
static std::vector<const Expr *> GetAncestorExpressions( ASTContext *Context, const Expr *Node )
{
    const Expr *stop = GetContainingSequenceStatement( Context, Node );
    std::vector<const Expr *> result;
    const Expr *next;
    do {
        next = nullptr;
        for( const DynTypedNode &parent : Context->getParents( *Node ) ) {
            if( const Expr *candidate = parent.get<Expr>() ) {
                next = candidate;
                break;
            }
        }
        if( next ) {
            result.push_back( next );
            Node = next;
        }
    } while( next && next != stop );

    return result;
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

    const Expr *ContainingStatement = GetContainingSequenceStatement( Result.Context, MemberCall );

    CallContext context{ ContainingStatement, On };
    calls_[context].push_back( { MemberCall, Result.Context } );
}

static bool IsEffectivelyConstCall( const CXXMemberCallExpr *Call )
{
    const CXXMethodDecl *method = Call->getMethodDecl();
    if( method->isConst() ) {
        return true;
    }
    DeclarationName name = method->getDeclName();
    DeclContextLookupResult similar_functions = method->getParent()->lookup( name );
    for( const NamedDecl *similar : similar_functions ) {
        if( const CXXMethodDecl *similar_function = dyn_cast<CXXMethodDecl>( similar ) ) {
            if( similar_function->isConst() ) {
                return true;
            }
        }
    }
    return false;
}

void UnsequencedCallsCheck::onEndOfTranslationUnit()
{
    for( std::pair<const CallContext, std::vector<CallWithASTContext>> &p : calls_ ) {
        const CallContext &context = p.first;
        std::vector<CallWithASTContext> &calls = p.second;
        std::sort( calls.begin(), calls.end() );
        calls.erase( std::unique( calls.begin(), calls.end() ), calls.end() );

        if( calls.size() < 2 ) {
            continue;
        }
        std::unordered_set<const CXXMemberCallExpr *> non_const_calls;
        for( const CallWithASTContext &member_call : calls ) {
            if( !IsEffectivelyConstCall( member_call.call ) ) {
                non_const_calls.insert( member_call.call );
            }
        }

        if( non_const_calls.empty() ) {
            continue;
        }

        // If some of the calls are an ancestor of other of the calls then we
        // might not need to report it.
        std::vector<CallWithASTContext> to_delete;
        for( const CallWithASTContext &member_call : calls ) {
            for( const Expr *ancestor :
                 GetAncestorExpressions( member_call.context, member_call.call ) ) {
                auto in_set = std::find( calls.begin(), calls.end(), ancestor );
                if( in_set == calls.end() ) {
                    continue;
                }
                // We have found a pair with an ancestor relationship.  Delete
                // the most const one.
                if( non_const_calls.count( in_set->call ) ) {
                    to_delete.push_back( member_call );
                } else {
                    to_delete.push_back( *in_set );
                }
            }
        }
        std::sort( to_delete.begin(), to_delete.end() );
        auto new_end = std::set_difference(
                           calls.begin(), calls.end(), to_delete.begin(), to_delete.end(), calls.begin() );
        calls.erase( new_end, calls.end() );
        if( calls.size() < 2 ) {
            continue;
        }

        const CXXRecordDecl *class_type = calls[0].call->getRecordDecl();

        diag(
            context.stmt->getBeginLoc(),
            "Unsequenced calls to member functions of %0 (of type %1), at least one of which is "
            "non-const."
        ) << context.on << class_type;

        for( const CallWithASTContext &member_call : calls ) {
            const CXXMethodDecl *method = member_call.call->getMethodDecl();
            if( IsEffectivelyConstCall( member_call.call ) ) {
                diag( member_call.call->getBeginLoc(), "Call to const member function %0",
                      DiagnosticIDs::Note ) << method;
            } else {
                diag( member_call.call->getBeginLoc(), "Call to non-const member function %0",
                      DiagnosticIDs::Note ) << method;
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

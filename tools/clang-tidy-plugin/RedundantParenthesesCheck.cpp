#include "RedundantParenthesesCheck.h"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <climits>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "Utils.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static auto isStmt()
{
    return stmt(
               anyOf(
                   ifStmt(), whileStmt(), switchStmt(), doStmt(), returnStmt()
               )
           );
}

static auto isStmtOrImplicitCastWithinStmt()
{
    return stmt(
               anyOf(
                   implicitCastExpr(
                       hasParent(
                           stmt(
                               anyOf(
                                   implicitCastExpr(
                                       hasParent(
                                           isStmt()
                                       )
                                   ),
                                   isStmt()
                               )
                           )
                       )
                   ),
                   isStmt()
               )
           );
}

void RedundantParenthesesCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        parenExpr(
            ignoringParens(
                expr(
                    anyOf(
                        callExpr( unless( cxxOperatorCallExpr() ) ),
                        characterLiteral(),
                        cxxBoolLiteral(),
                        cxxNullPtrLiteralExpr(),
                        declRefExpr(),
                        fixedPointLiteral(),
                        floatLiteral(),
                        imaginaryLiteral(),
                        integerLiteral(),
                        stringLiteral(),
                        userDefinedLiteral()
                    )
                ).bind( "child_expr" )
            ),
            unless( hasParent( unaryExprOrTypeTraitExpr() ) )
        ).bind( "paren_expr" ),
        this
    );
    Finder->addMatcher(
        varDecl(
            hasInitializer(
                parenExpr( ignoringParens( expr().bind( "child_expr" ) ) ).bind( "paren_expr" )
            )
        ),
        this
    );
    Finder->addMatcher(
        parenExpr( hasParent( parenExpr().bind( "paren_expr" ) ) ).bind( "child_expr" ),
        this
    );
    Finder->addMatcher(
        expr(
            hasParent(
                parenExpr( hasParent( isStmtOrImplicitCastWithinStmt() ) ).bind( "paren_expr" )
            ),
            unless( binaryOperator( isAssignmentOperator() ) )
        ).bind( "child_expr" ),
        this
    );
}

static void CheckExpr( RedundantParenthesesCheck &Check, const MatchFinder::MatchResult &Result )
{
    const Expr *OuterExpr = Result.Nodes.getNodeAs<Expr>( "paren_expr" );
    const Expr *InnerExpr = Result.Nodes.getNodeAs<Expr>( "child_expr" );
    if( !OuterExpr || !InnerExpr ) {
        return;
    }

    SourceRange OuterRange = OuterExpr->getSourceRange();
    SourceRange InnerRange = InnerExpr->getSourceRange();

    const SourceManager *SM = Result.SourceManager;
    SourceLocation ExpansionBegin = SM->getFileLoc( OuterRange.getBegin() );
    if( ExpansionBegin != OuterRange.getBegin() ) {
        // This means we're inside a macro expansion
        return;
    }

    StringRef Replacement = getText( Result, InnerRange );

    Check.diag( OuterExpr->getBeginLoc(), "Redundant parentheses." ) <<
            FixItHint::CreateReplacement( OuterRange, Replacement );
}

void RedundantParenthesesCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckExpr( *this, Result );
}

} // namespace clang::tidy::cata

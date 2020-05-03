#include "UseLocalizedSortingCheck.h"

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

namespace clang
{
namespace tidy
{
namespace cata
{

void UseLocalizedSortingCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxOperatorCallExpr(
            hasArgument(
                0,
                expr(
                    hasType(
                        qualType(
                            anyOf( asString( "const std::string" ), asString( "std::string" ) )
                        ).bind( "arg0Type" )
                    )
                ).bind( "arg0Expr" )
            ),
            hasOverloadedOperatorName( "<" )
        ).bind( "call" ),
        this
    );
}

static void CheckCall( UseLocalizedSortingCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CXXOperatorCallExpr *Call = Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "call" );
    const QualType *Arg0Type = Result.Nodes.getNodeAs<QualType>( "arg0Type" );
    const Expr *Arg0Expr = Result.Nodes.getNodeAs<Expr>( "arg0Expr" );
    if( !Call || !Arg0Type || !Arg0Expr ) {
        return;
    }

    StringRef Arg0Text = getText( Result, Arg0Expr );
    if( Arg0Text.endswith( "id" ) ) {
        return;
    }

    Check.diag( Call->getBeginLoc(),
                "Raw comparison of %0.  For UI purposes please use localized_compare from "
                "translations.h." ) << *Arg0Type;
}

void UseLocalizedSortingCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckCall( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang

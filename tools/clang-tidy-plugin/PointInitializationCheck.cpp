#include "PointInitializationCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{
void PointInitializationCheck::registerMatchers( MatchFinder *Finder )
{
    using CxxConstructorMatcher = clang::ast_matchers::internal::Matcher<Expr>;
    const CxxConstructorMatcher ZeroConstructor = cxxConstructExpr( anyOf(
                allOf(
                    argumentCountIs( 1 ),
                    hasArgument( 0, declRefExpr( to( varDecl(
                                     anyOf( hasName( "point_zero" ), hasName( "tripoint_zero" ) )
                                 ) ) ) ) ),
                allOf(
                    hasArgument( 0, integerLiteral( equals( 0 ) ) ),
                    hasArgument( 1, integerLiteral( equals( 0 ) ) ),
                    anyOf(
                        argumentCountIs( 2 ),
                        allOf(
                            hasArgument( 2, integerLiteral( equals( 0 ) ) ),
                            argumentCountIs( 3 )
                        )
                    )
                )
            ) ).bind( "expr" );
    Finder->addMatcher( varDecl(
                            unless( parmVarDecl() ),
                            hasType( isPointType() ),
                            hasInitializer( ZeroConstructor )
                        ).bind( "decl" ), this );
    Finder->addMatcher( cxxCtorInitializer(
                            forField( hasType( isPointType() ) ),
                            withInitializer( ZeroConstructor )
                        ).bind( "init" ), this );
}

static void CheckDecl( PointInitializationCheck &Check, const MatchFinder::MatchResult &Result )
{
    const VarDecl *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !MatchedDecl ) {
        return;
    }
    const Expr *MatchedExpr = Result.Nodes.getNodeAs<Expr>( "expr" );
    if( !MatchedExpr ) {
        return;
    }
    QualType Type = MatchedDecl->getType();
    PrintingPolicy Policy( LangOptions{} );
    Policy.adjustForCPlusPlus();
    Type.removeLocalConst();
    std::string Replacement = Type.getAsString( Policy ) + " " + MatchedDecl->getNameAsString();
    SourceRange ToReplace( MatchedDecl->getTypeSpecStartLoc(),  MatchedDecl->getEndLoc() );
    Check.diag(
        MatchedExpr->getBeginLoc(),
        "Unnecessary initialization of %0. %1 is zero-initialized by default." )
            << MatchedDecl << Type
            << FixItHint::CreateReplacement( ToReplace, Replacement );
}

static void CheckInit( PointInitializationCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CXXCtorInitializer *MatchedInit = Result.Nodes.getNodeAs<CXXCtorInitializer>( "init" );
    if( !MatchedInit ) {
        return;
    }
    FieldDecl *Member = MatchedInit->getMember();
    if( !Member ) {
        return;
    }
    QualType Type = Member->getType();
    Check.diag(
        MatchedInit->getInit()->getBeginLoc(),
        "Unnecessary initialization of %0. %1 is zero-initialized by default." ) <<
                Member << Type;
}

void PointInitializationCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
    CheckInit( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang

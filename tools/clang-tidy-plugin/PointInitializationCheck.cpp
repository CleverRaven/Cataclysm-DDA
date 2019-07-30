#include "PointInitializationCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{
void PointInitializationCheck::registerMatchers( MatchFinder *Finder )
{
    using TypeMatcher = clang::ast_matchers::internal::Matcher<QualType>;
    const TypeMatcher IsPointType =
        qualType( anyOf( asString( "struct point" ), asString( "struct tripoint" ) ) );
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
                            hasType( IsPointType ),
                            hasInitializer( ZeroConstructor )
                        ).bind( "decl" ), this );
    Finder->addMatcher( cxxCtorInitializer(
                            forField( hasType( IsPointType ) ),
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
    Check.diag(
        MatchedExpr->getBeginLoc(),
        "Unnecessary initialization of %0. %1 is zero-initialized by default." ) <<
                MatchedDecl << Type;
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

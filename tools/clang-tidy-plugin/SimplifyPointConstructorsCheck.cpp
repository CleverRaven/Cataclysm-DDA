#include "SimplifyPointConstructorsCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

static auto isPointConstructor()
{
    return cxxConstructorDecl(
               ofClass( cxxRecordDecl( anyOf( hasName( "point" ), hasName( "tripoint" ) ) ) )
           );
}

static auto isMemberExpr( const std::string &type, const std::string &member_,
                          const std::string &objBind )
{
    return ignoringParenCasts(
               memberExpr(
                   member( hasName( member_ ) ),
                   hasObjectExpression(
                       expr( hasType( cxxRecordDecl( hasName( type ) ) ) ).bind( objBind )
                   )
               )
           );
}

void SimplifyPointConstructorsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            hasArgument( 0, isMemberExpr( "point", "x", "xobj" ) ),
            hasArgument( 1, isMemberExpr( "point", "y", "yobj" ) )
        ).bind( "constructorCall" ),
        this
    );
}

static void CheckPointFromPoint( SimplifyPointConstructorsCheck &Check,
                                 const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const Expr *XExpr = Result.Nodes.getNodeAs<Expr>( "xobj" );
    const Expr *YExpr = Result.Nodes.getNodeAs<Expr>( "yobj" );
    if( !ConstructorCall || !ConstructorDecl || !XExpr || !YExpr ) {
        return;
    }

    std::string ReplacementX = getText( Result, XExpr );
    std::string ReplacementY = getText( Result, YExpr );

    if( ReplacementX != ReplacementY ) {
        return;
    }

    SourceRange SourceRangeToReplace( ConstructorCall->getArg( 0 )->getBeginLoc(),
                                      ConstructorCall->getArg( 1 )->getEndLoc() );

    if( const CXXTemporaryObjectExpr *T = dyn_cast<CXXTemporaryObjectExpr>( ConstructorCall ) ) {
        if( ConstructorDecl->getNumParams() == 2 ) {
            SourceRangeToReplace = T->getSourceRange();
        }
    }

    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    Check.diag(
        ConstructorCall->getBeginLoc(),
        "Construction of point can be simplified." ) <<
                FixItHint::CreateReplacement( CharRangeToReplace, ReplacementX );
    /*Check.diag( YArg->getBeginLoc(), "y arg", DiagnosticIDs::Note );
    Check.diag( YParam->getLocation(), "y param", DiagnosticIDs::Note );
    if( ZParam ) {
        Check.diag( ZArg->getBeginLoc(), "z arg", DiagnosticIDs::Note );
    }
    Check.diag( NewCallee->getLocation(), "alternate overload", DiagnosticIDs::Note );*/
}

void SimplifyPointConstructorsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckPointFromPoint( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang

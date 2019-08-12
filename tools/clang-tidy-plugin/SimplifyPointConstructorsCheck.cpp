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
            testWhetherConstructingTemporary(),
            hasArgument( 0, isMemberExpr( "point", "x", "xobj" ) ),
            hasArgument( 1, isMemberExpr( "point", "y", "yobj" ) )
        ).bind( "constructorCallFromPoint" ),
        this
    );
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            testWhetherConstructingTemporary(),
            hasArgument( 0, isMemberExpr( "tripoint", "x", "xobj" ) ),
            hasArgument( 1, isMemberExpr( "tripoint", "y", "yobj" ) ),
            anyOf(
                hasArgument( 2, isMemberExpr( "tripoint", "z", "zobj" ) ),
                anything()
            )
        ).bind( "constructorCallFromTripoint" ),
        this
    );
}

static void CheckFromPoint( SimplifyPointConstructorsCheck &Check,
                            const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCallFromPoint" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const MaterializeTemporaryExpr *TempParent =
        Result.Nodes.getNodeAs<MaterializeTemporaryExpr>( "temp" );
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

    if( TempParent ) {
        if( ConstructorDecl->getNumParams() == 2 ) {
            SourceRangeToReplace = ConstructorCall->getSourceRange();
        }
    }

    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    Check.diag(
        ConstructorCall->getBeginLoc(),
        "Construction of %0 can be simplified." ) << ConstructorDecl->getParent() <<
                FixItHint::CreateReplacement( CharRangeToReplace, ReplacementX );
}

static void CheckFromTripoint( SimplifyPointConstructorsCheck &Check,
                               const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCallFromTripoint" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const Expr *TempParent = Result.Nodes.getNodeAs<Expr>( "temp" );
    const Expr *XExpr = Result.Nodes.getNodeAs<Expr>( "xobj" );
    const Expr *YExpr = Result.Nodes.getNodeAs<Expr>( "yobj" );
    const Expr *ZExpr = Result.Nodes.getNodeAs<Expr>( "zobj" );
    if( !ConstructorCall || !ConstructorDecl || !XExpr || !YExpr ) {
        return;
    }

    std::string ReplacementX = getText( Result, XExpr );
    std::string ReplacementY = getText( Result, YExpr );

    if( ReplacementX != ReplacementY ) {
        return;
    }

    std::string ReplacementZ;
    unsigned int MaxArg = 1;

    if( ZExpr ) {
        ReplacementZ = getText( Result, ZExpr );
        if( ReplacementZ == ReplacementX ) {
            MaxArg = 2;
        } else {
            ReplacementZ.clear();
        }
    }

    if( MaxArg == 1 ) {
        ReplacementX += ".xy()";
    }

    SourceRange SourceRangeToReplace( ConstructorCall->getArg( 0 )->getBeginLoc(),
                                      ConstructorCall->getArg( MaxArg )->getEndLoc() );

    if( TempParent ) {
        if( ConstructorDecl->getNumParams() == MaxArg + 1 ) {
            SourceRangeToReplace = ConstructorCall->getSourceRange();
        }
    }

    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    Check.diag(
        ConstructorCall->getBeginLoc(),
        "Construction of %0 can be simplified." ) << ConstructorDecl->getParent() <<
                FixItHint::CreateReplacement( CharRangeToReplace, ReplacementX );
}

void SimplifyPointConstructorsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckFromPoint( *this, Result );
    CheckFromTripoint( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang

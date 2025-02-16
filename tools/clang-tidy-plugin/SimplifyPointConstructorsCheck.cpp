#include "SimplifyPointConstructorsCheck.h"

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

namespace clang::tidy::cata
{

static auto isMemberExpr( const std::string_view/*type*/, const std::string &member_,
                          const std::string &objBind )
{
    return ignoringParenCasts(
               memberExpr(
                   member( hasName( member_ ) ),
                   hasObjectExpression( expr().bind( objBind ) )
               )
           );
}

void SimplifyPointConstructorsCheck::registerMatchers( MatchFinder *Finder )
{
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
        ).bind( "constructorCall" ),
        this
    );
}

struct ExpressionCategory {
    ExpressionCategory() = default;

    ExpressionCategory( const MatchFinder::MatchResult &Result, const Expr *E ) :
        Replacement( getText( Result, E ) ) {
        if( StringRef( Replacement ).ends_with( "->" ) ) {
            Replacement.erase( Replacement.end() - 2, Replacement.end() );
        }
        QualType EType = E->getType();
        if( EType->isPointerType() ) {
            IsArrowRef = true;
            if( const CXXRecordDecl *Record = EType->getPointeeCXXRecordDecl() ) {
                IsPoint = isPointType( Record );
                IsTripoint = Record->getName() == "tripoint";
            }
        } else if( const CXXRecordDecl *Record = EType->getAsCXXRecordDecl() ) {
            IsPoint = isPointType( Record );
            IsTripoint = Record->getName() == "tripoint";
        }
    }

    bool IsPoint = false;
    bool IsTripoint = false;
    bool IsArrowRef = false;
    std::string Replacement;

    std::tuple<bool, bool, bool, std::string> asTuple() const {
        return std::make_tuple( IsPoint, IsTripoint, IsArrowRef, Replacement );
    }

    friend bool operator==( const ExpressionCategory &l, const ExpressionCategory &r ) {
        return l.asTuple() == r.asTuple();
    }
    friend bool operator!=( const ExpressionCategory &l, const ExpressionCategory &r ) {
        return l.asTuple() != r.asTuple();
    }
};

static void CheckConstructor( SimplifyPointConstructorsCheck &Check,
                              const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const MaterializeTemporaryExpr *TempParent =
        Result.Nodes.getNodeAs<MaterializeTemporaryExpr>( "temp" );
    const Expr *XExpr = Result.Nodes.getNodeAs<Expr>( "xobj" );
    const Expr *YExpr = Result.Nodes.getNodeAs<Expr>( "yobj" );
    const Expr *ZExpr = Result.Nodes.getNodeAs<Expr>( "zobj" );
    if( !ConstructorCall || !ConstructorDecl || !XExpr || !YExpr ) {
        return;
    }

    ExpressionCategory XCat( Result, XExpr );
    ExpressionCategory YCat( Result, YExpr );

    if( !XCat.IsPoint || !YCat.IsPoint ) {
        return;
    }

    if( XCat != YCat ) {
        return;
    }

    ExpressionCategory ZCat;
    unsigned int MaxArg = 1;

    if( ZExpr ) {
        ZCat = ExpressionCategory( Result, ZExpr );
        if( ZCat == XCat ) {
            MaxArg = 2;
        } else {
            ZCat = ExpressionCategory();
        }
    }

    std::string Replacement;

    if( MaxArg == 1 && XCat.IsTripoint ) {
        if( XCat.IsArrowRef ) {
            Replacement = XCat.Replacement + "->xy()";
        } else {
            Replacement = XCat.Replacement + ".xy()";
        }
    } else {
        if( XCat.IsArrowRef ) {
            Replacement = "*" + XCat.Replacement;
        } else {
            Replacement = XCat.Replacement;
        }
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
                FixItHint::CreateReplacement( CharRangeToReplace, Replacement );
}

void SimplifyPointConstructorsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
}

} // namespace clang::tidy::cata

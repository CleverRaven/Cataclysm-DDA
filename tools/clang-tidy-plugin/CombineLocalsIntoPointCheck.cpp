#include "CombineLocalsIntoPointCheck.h"

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

void CombineLocalsIntoPointCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl( hasType( isInteger() ), isXParam() ).bind( "xdecl" ),
        this
    );
    Finder->addMatcher(
        declRefExpr(
            hasDeclaration( varDecl( hasType( isInteger() ) ).bind( "decl" ) )
        ).bind( "declref" ),
        this
    );
}

static void CheckDecl( CombineLocalsIntoPointCheck &Check, const MatchFinder::MatchResult &Result )
{
    const VarDecl *XDecl = Result.Nodes.getNodeAs<VarDecl>( "xdecl" );
    if( !XDecl ) {
        return;
    }
    const Decl *NextDecl = XDecl->getNextDeclInContext();
    const VarDecl *YDecl = dyn_cast_or_null<VarDecl>( NextDecl );
    if( !YDecl ) {
        return;
    }
    NextDecl = YDecl->getNextDeclInContext();
    const VarDecl *ZDecl = dyn_cast_or_null<VarDecl>( NextDecl );

    NameConvention NameMatcher( XDecl->getName() );

    if( !NameMatcher || NameMatcher.Match( YDecl->getName() ) != NameConvention::YName ) {
        return;
    }

    if( ZDecl && NameMatcher.Match( ZDecl->getName() ) != NameConvention::ZName ) {
        ZDecl = nullptr;
    }

    const Expr *XInit = XDecl->getAnyInitializer();
    const Expr *YInit = YDecl->getInit();
    const Expr *ZInit = ZDecl ? ZDecl->getInit() : nullptr;

    if( !XInit || !YInit || ( ZDecl && !ZInit ) ) {
        return;
    }

    std::string NewVarName = NameMatcher.getRoot();
    if( NewVarName.empty() ) {
        NewVarName = "p";
    }

    Check.usageReplacements.emplace( XDecl, NewVarName + ".x" );
    Check.usageReplacements.emplace( YDecl, NewVarName + ".y" );
    if( ZDecl ) {
        Check.usageReplacements.emplace( ZDecl, NewVarName + ".z" );
    }

    // Construct replacement text
    std::string Replacement =
        ( "point " + NewVarName + "( " + getText( Result, XInit ) + ", " +
          getText( Result, YInit ) ).str();
    if( ZDecl ) {
        Replacement = ( "tri" + Replacement + ", " + getText( Result, ZInit ) ).str();
    }
    Replacement += " )";

    if( XDecl->getType().isConstQualified() ) {
        Replacement = "const " + Replacement;
    }

    SourceRange RangeToReplace( XDecl->getBeginLoc(), YDecl->getEndLoc() );
    Check.diag( YDecl->getLocation(), "%0 variable", DiagnosticIDs::Note ) << YDecl;
    Check.diag( XDecl->getBeginLoc(),
                "Variables %0 and %1 could be combined into a single 'point' variable." )
            << XDecl << YDecl << FixItHint::CreateReplacement( RangeToReplace, Replacement );
}

static void CheckDeclRef( CombineLocalsIntoPointCheck &Check,
                          const MatchFinder::MatchResult &Result )
{
    const DeclRefExpr *DeclRef = Result.Nodes.getNodeAs<DeclRefExpr>( "declref" );
    const VarDecl *Decl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !DeclRef || !Decl ) {
        return;
    }

    auto Replacement = Check.usageReplacements.find( Decl );
    if( Replacement == Check.usageReplacements.end() ) {
        return;
    }

    Check.diag( DeclRef->getBeginLoc(), "Update %0 to '%1'.", DiagnosticIDs::Note )
            << Decl << Replacement->second
            << FixItHint::CreateReplacement( DeclRef->getSourceRange(), Replacement->second );
}

void CombineLocalsIntoPointCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
    CheckDeclRef( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
